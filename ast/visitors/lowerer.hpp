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

using LowererError = std::variant<ScopeStackError>; //, TypeError>;

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
    llvm::AllocaInst *memory;

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
    llvm::Function *llvm_function;
  };

  using PascalIdent = std::string;

  // builtins:
private:
  LowererErrorOr<void> declare_builtins();
  LowererErrorOr<void> declare_builtin_types();
  LowererErrorOr<void> declare_builtin_intio();
  LowererErrorOr<void> declare_builtin_strio();

  // declaration helpers
private:
  // Creates alloca inst and stores the variable in scope stack.
  // Returns scope stack error, if a redefinition in the same scope, for
  // example.
  LowererErrorOr<void> declare_var(Variable var);

  // Just store the type in scope stack.
  // Type declaration is always a definition. There's no
  //   declaration without initialization.
  //   Whereas for variables declaration is not an assignment.
  //   Although they are constructed with some meaningful value.
  //   Regarding functions, there are forward declarations (will be in future).
  // Returns scope stack error, if a redefinition in the same scope, for
  // example.
  LowererErrorOr<void> declare_type(Type type);

  // For functions we have not only to store them in scope stack,
  //   but tell llvm such a function exist, also convert pascal
  //   types to llvm ones.
  LowererErrorOr<void> declare_func(Function func);

  // evalution functions for expressions.
private:
  // TODO: add const to all references to ast.
  LowererErrorOr<llvm::Value *> eval(const pas::ast::FuncCall &func_call);
  LowererErrorOr<llvm::Value *> eval(bool value);
  LowererErrorOr<llvm::Value *> eval(int value);
  LowererErrorOr<llvm::Value *> eval(const std::string value);
  LowererErrorOr<llvm::Value *> eval(const pas::ast::Nil &value);
  LowererErrorOr<llvm::Value *> eval(const pas::ast::Negation &value);

  LowererErrorOr<llvm::Value *> eval(const pas::ast::Factor &factor);
  LowererErrorOr<llvm::Value *> eval(const pas::ast::Term &term);
  LowererErrorOr<llvm::Value *> eval(const pas::ast::SimpleExpr &simple_expr);
  LowererErrorOr<llvm::Value *> eval(const pas::ast::Expr &expr);

  LowererErrorOr<llvm::Value *>
  eval_read_int(const pas::ast::FuncCall &func_call);

  LowererErrorOr<llvm::Value *> eval(const pas::ast::Designator &value);

  // visit functions for the toplevel scope.
private:
  LowererErrorOr<void> visit(pas::ast::CompilationUnit &cu);
  LowererErrorOr<void> visit(pas::ast::ProgramModule &pm);
  LowererErrorOr<void> visit_toplevel(pas::ast::Block &block);

  // rename to visit_toplevel_*
  LowererErrorOr<void> process_decls(pas::ast::Declarations &decls);
  LowererErrorOr<void> process_type_def(pas::ast::TypeDef &type_def);
  LowererErrorOr<void> process_var_decl(pas::ast::VarDecl &var_decl);

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
