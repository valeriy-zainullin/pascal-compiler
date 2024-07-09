#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

#include "ast/ast.hpp"
#include "ast/utils/get_idx.hpp"
#include "support/computed_type.hpp"
#include "support/error.hpp"
#include "support/scopes.hpp"

namespace pas {
namespace visitor {

// Примеры IR-а.
//   https://mcyoung.xyz/2023/08/01/llvm-ir/

struct CallError {
  enum class Reason {
    WrongNumberOfArgs,
    ArgTypeMismatch,
  } reason;
  std::string description;
};

struct TypeError {
  enum class Reason {
    IncompatibleTypesInCmp,
    InvalidTypesInMath,
    TypeMismatchInAssignment,
    ArrayAccessorIsNotInteger,
    IncompatibleTypesInMultOp,
    IncompatibleTypesInAddOp,
  } reason;
  std::string description;
};

struct AccessError {
  enum class Reason {
    UnsupportedAccessType,
    WrongNumberOfArrayAccessors,
  } reason;
  std::string description;
};

struct NotImplementedError {
  std::string description;
};

// Можно в будущем сделать класс IRGenerator.
//   Он принимает в себя шаблон, на который будет хранить ссылку.
//   Шаблонный параметр будет называться Specialization.
//   И у него будут все эти методы, которые есть у Lowerer.
//   И для разных IR по сути лишь специализацию заменять.
// Интерпретатора in-place больше не будет, будет интерпретация IR.
// Можно сделать плагин языка pascal для vscode, который будет проверять
//   синтаксис и семантику с помощью моего компилятора. Юнитов нет, плагин
//   поддерживает только один файл. Будет вызывать утилиту
//   pas-typechecker, а она вызывает Typechecker visitor. Этот visitor
//   тоже наследуется от IRGenerator, просто никакой кодогенерации не
//   происходит, вместо дерева IR выдается std::monostate.

using LowererError = std::variant<ScopeStackError, CallError, TypeError,
                                  AccessError, NotImplementedError>;

template <typename ValueType>
using LowererErrorOr = ErrorOr<LowererError, ValueType>;

class Lowerer {
public:
  static void initialize_for_native_target() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
  }

  // filepath is absolute path.
  Lowerer(llvm::LLVMContext &context, const std::string &filepath,
          pas::ast::CompilationUnit &cu);

  std::vector<LowererError> release_errors();

  std::unique_ptr<llvm::Module> release_module();

  // Useful typedefs.
private:
  struct TmpValue {
    // llvm::Value* is actually an
    //   evaluation tree (made of instructions and value declarations) for the
    //   value.
    //   For example, to evaluate "f(var_name + 2)" we would have such a tree:
    //   llvm::FunctionCall f
    //     llvm::AddInst
    //       llvm::LoadInst %var_name
    //     llvm::Int 2
    // which is the same as this:
    //   %1 = load int %var_name
    //   %2 = add %1, int 2
    //   call
    // TODO: complete this note!
    llvm::Value *value;
    pas::ComputedType type;
  };

  struct Variable : public pas::ScopeStackInterface::BasicVariable {
    llvm::Value
        *memory; // Always a llvm::AllocaInst* or a llvm::GlobalVariable*.

    void start_lifetime(llvm::Module *module, llvm::IRBuilder<> *ir_builder) {
      if (type == BasicType::String) {
        // Call __create_string to allocate
        //   a std::string* and initialize it.

        // https://stackoverflow.com/a/22310371
        llvm::Function *init_func = module->getFunction("__create_string");
        assert(init_func != nullptr);
        std::vector<llvm::Value *> args = {memory};
        ir_builder->CreateCall(init_func, args);
      }
    }

    void end_lifetime(llvm::Module *module, llvm::IRBuilder<> *ir_builder) {
      if (type == BasicType::String) {
        // Call __destroy_string on std::string*
        //   to destroy a std::string and
        //   deallocate the memory.

        // https://stackoverflow.com/a/22310371
        llvm::Function *deinit_func = module->getFunction("__destroy_string");
        assert(deinit_func != nullptr);
        std::vector<llvm::Value *> args = {memory};
        ir_builder->CreateCall(deinit_func, args);
      }
    }
  };

  // Пока не нужно хранить дополнительную информацию рядом с
  //   объявлением типа.
  using Type = pas::ScopeStackInterface::BasicType;

  struct Function : public pas::ScopeStackInterface::BasicFunction {
    llvm::Function *llvm_function = nullptr;
  };

  using PascalIdent = std::string;

  // builtins:
private:
  LowererErrorOr<void> declare_builtins();
  LowererErrorOr<void> declare_builtin_types();
  LowererErrorOr<void> declare_builtin_intio();
  LowererErrorOr<void> declare_builtin_strio();

  // declaration helpers
  //   These do not accept structs, but rather
  //   the arguments themselves. Because they'd also
  //   fill some fields. And usually calling don't need
  //   that info, it'd just query scope stack, when it's
  //   needed.
  //   ---
  //   Also I feel like I don't like modifying fields by reference
  //   argument or pointer argument. Return values are better.
  //   Or I'd do methods instead. Because it's encapsulation.
  //   At the very least, I can make a friend function. But
  //   such access should be hidden.
  //   It's only about C++ and direct access. Also it's not possible
  //   for private fields.. So it's already done. Maybe I wouldn't have
  //   any public fields, in the first place. That's a good guideline.
  //   - Only a reason may make me do a public field for a class (not
  //   C-like struct, an aggregate type). Implementation should be
  //   hidden. At least, if I have time to do that.
  //   I need to write these guidelines for myself somewhere.
private:
  // Creates alloca inst and stores the variable in scope stack.
  // Returns scope stack error, if it's, for example, a redefinition
  //   in the same scope.
  LowererErrorOr<void> declare_var(std::string name, pas::ComputedType type);

  // Just store the type in scope stack.
  // Type declaration is always a definition. There's no
  //   declaration without initialization.
  //   Whereas for variables declaration is not an assignment.
  //   Although they are constructed with some meaningful value.
  //   Regarding functions, there are forward declarations (will be in future).
  // Returns scope stack error, if a redefinition in the same scope, for
  // example.
  LowererErrorOr<void> declare_type(std::string name, pas::ComputedType type);

  // For functions we have not only to store them in scope stack,
  //   but tell llvm such a function exist, also convert pascal
  //   types to llvm ones.
  // We accept copies of computed types, because we'd copy them
  //   anyway..
  LowererErrorOr<void> declare_func(std::string name,
                                    std::optional<pas::ComputedType> ret_type,
                                    std::vector<pas::ComputedType> args = {});

  // evalution functions for expressions.
private:
  LowererErrorOr<TmpValue> eval_op_int(TmpValue lhs, pas::ast::RelOp op,
                                       TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_bool(TmpValue lhs, pas::ast::RelOp op,
                                        TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_char(TmpValue lhs, pas::ast::RelOp op,
                                        TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_real(TmpValue lhs, pas::ast::RelOp op,
                                        TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_str(TmpValue lhs, pas::ast::RelOp op,
                                       TmpValue rhs);

  LowererErrorOr<TmpValue> eval_op_record(TmpValue lhs, pas::ast::RelOp op,
                                          TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_set(TmpValue lhs, pas::ast::RelOp op,
                                       TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_array(TmpValue lhs, pas::ast::RelOp op,
                                         TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_pointer(TmpValue lhs, pas::ast::RelOp op,
                                           TmpValue rhs);

  LowererErrorOr<TmpValue> eval_op_int(TmpValue lhs, pas::ast::MultOp op,
                                       TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_bool(TmpValue lhs, pas::ast::MultOp op,
                                        TmpValue rhs);

  LowererErrorOr<TmpValue> eval_op_int(TmpValue lhs, pas::ast::AddOp op,
                                       TmpValue rhs);
  LowererErrorOr<TmpValue> eval_op_bool(TmpValue lhs, pas::ast::AddOp op,
                                        TmpValue rhs);

  LowererErrorOr<TmpValue> eval_not_int(TmpValue value);
  LowererErrorOr<TmpValue> eval_not_bool(TmpValue value);

  LowererErrorOr<TmpValue>
  eval_access_str(TmpValue value, const pas::ast::DesignatorItem &item);
  LowererErrorOr<TmpValue>
  eval_access_record(TmpValue value, const pas::ast::DesignatorItem &item);
  LowererErrorOr<TmpValue>
  eval_access_set(TmpValue value, const pas::ast::DesignatorItem &item);
  LowererErrorOr<TmpValue>
  eval_access_array(TmpValue value, const pas::ast::DesignatorItem &item);
  LowererErrorOr<TmpValue>
  eval_access_pointer(TmpValue value, const pas::ast::DesignatorItem &item);

  // TODO: add const to all references to ast.
  LowererErrorOr<TmpValue> eval(const pas::ast::FuncCall &func_call);
  LowererErrorOr<TmpValue> eval(bool value);
  LowererErrorOr<TmpValue> eval(int value);
  LowererErrorOr<TmpValue> eval(const std::string value);
  LowererErrorOr<TmpValue> eval(const pas::ast::Nil &value);
  LowererErrorOr<TmpValue> eval(const pas::ast::Negation &value);

  LowererErrorOr<TmpValue> eval(const pas::ast::Factor &factor);
  LowererErrorOr<TmpValue> eval(const pas::ast::Term &term);
  LowererErrorOr<TmpValue> eval(const pas::ast::SimpleExpr &simple_expr);
  LowererErrorOr<TmpValue> eval(const pas::ast::Expr &expr);

  LowererErrorOr<TmpValue> eval(const pas::ast::Designator &value);

  // visit functions for the topmost scope.
private:
  LowererErrorOr<void> visit(const pas::ast::CompilationUnit &cu);
  LowererErrorOr<void> visit(const pas::ast::ProgramModule &pm);
  LowererErrorOr<void> visit_topmost(const pas::ast::Block &block);

  LowererErrorOr<void> visit_topmost(const pas::ast::Declarations &decls);

  // visit functions for declarations (declares such variables in scope table).
private:
  LowererErrorOr<void> visit(const pas::ast::Declarations &decls);
  LowererErrorOr<void> visit(const pas::ast::TypeDef &type_def);
  LowererErrorOr<void> visit(const pas::ast::VarDecl &var_decl);

  // visit functions for statements inside a block
private:
  LowererErrorOr<void> visit(const pas::ast::MemoryStmt &memory_stmt);
  LowererErrorOr<void> visit(const pas::ast::RepeatStmt &repeat_stmt);
  LowererErrorOr<void> visit(const pas::ast::CaseStmt &case_stmt);
  LowererErrorOr<void> visit(const pas::ast::StmtSeq &stmt_seq);
  LowererErrorOr<void> visit(const pas::ast::IfStmt &if_stmt);
  LowererErrorOr<void> visit(const pas::ast::EmptyStmt &empty_stmt);
  LowererErrorOr<void> visit(const pas::ast::ForStmt &for_stmt);
  LowererErrorOr<void> visit(const pas::ast::Assignment &assignment);
  LowererErrorOr<void> visit(const pas::ast::ProcCall &proc_call);
  LowererErrorOr<void> visit(const pas::ast::WhileStmt &while_stmt);

  LowererErrorOr<void> visit_write_int(const pas::ast::ProcCall &proc_call);
  LowererErrorOr<void> visit_write_str(const pas::ast::ProcCall &proc_call);

  // Scope lifetime functions and lookup.
  //   - Variable creation, destruction.
  // Scope lookup (find function, typedef or variable by identifier)
private:
  llvm::Type *get_llvm_type(const pas::ComputedType &lang_type);

  // llvm related fields
private:
  template <typename T> struct EraseFromParent {
    void operator()(T *ptr) { ptr->eraseFromParent(); }
  };

  llvm::LLVMContext &context_;
  std::unique_ptr<llvm::Module> module_uptr_;

  llvm::Function *current_func_ = nullptr;
  llvm::IRBuilder<> *ir_builder_ = nullptr;

  // static EraseFromParent<llvm::Function> FunctionDeleter;
  // std::unique_ptr<llvm::Function, decltype(FunctionDeleter)> main_func_uptr_;

  // pascal related fields.
private:
  pas::ScopeStack<Variable, Type, Function> scopes_;

  // Чтобы посмотреть в действии, как работает трансляция, посмотрите видео
  // Андреаса Клинга.
  //   Он делал jit-компилятор javascript в браузере ladybird.
  //   https://www.youtube.com/watch?v=8mxubNQC5O8
};

} // namespace visitor
} // namespace pas
