#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

#include "ast/ast.hpp"
#include "ast/utils/get_idx.hpp"
#include "support/computed_type.hpp"
#include "support/scopes.hpp"

namespace pas {
namespace visitor {

// Примеры IR-а.
//   https://mcyoung.xyz/2023/08/01/llvm-ir/

class Lowerer {
public:
  static void initialize_for_native_target() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
  }

  Lowerer(llvm::LLVMContext &context, const std::string &file_name,
          pas::ast::CompilationUnit &cu);

  std::unique_ptr<llvm::Module> release_module();

  // Useful typedefs.
private:
  struct Value {
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

  struct Variable : public pas::BasicVariable {
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

  using PascalIdent = std::string;

  // evalution functions for expressions.
private:
  llvm::Value *eval(pas::ast::FuncCall &func_call);
  llvm::Value *eval(pas::ast::Factor &factor);
  llvm::Value *eval(pas::ast::Term &term);
  llvm::Value *eval(pas::ast::SimpleExpr &simple_expr);
  llvm::Value *eval(pas::ast::Expr &expr);

  llvm::Value *eval_read_int(pas::ast::FuncCall &func_call);

  // visit functions for the toplevel scope.
private:
  void visit(pas::ast::CompilationUnit &cu);
  void visit(pas::ast::ProgramModule &pm);
  void visit_toplevel(pas::ast::Block &block);

  // rename to visit_toplevel_*
  void process_decls(pas::ast::Declarations &decls);
  void process_type_def(pas::ast::TypeDef &type_def);
  void process_var_decl(pas::ast::VarDecl &var_decl);

  // visit functions for statements inside a block
private:
  void visit(pas::ast::MemoryStmt &memory_stmt);
  void visit(pas::ast::RepeatStmt &repeat_stmt);
  void visit(pas::ast::CaseStmt &case_stmt);
  void visit(pas::ast::StmtSeq &stmt_seq);
  void visit(pas::ast::IfStmt &if_stmt);
  void visit(pas::ast::EmptyStmt &empty_stmt);
  void visit(pas::ast::ForStmt &for_stmt);
  void visit(pas::ast::Assignment &assignment);
  void visit(pas::ast::ProcCall &proc_call);
  void visit(pas::ast::WhileStmt &while_stmt);

  void visit_write_int(pas::ast::ProcCall &proc_call);
  void visit_write_str(pas::ast::ProcCall &proc_call);

  // Scope lifetime functions and lookup.
  //   - Variable creation, destruction.
  // Scope lookup (find function, typedef or variable by identifier)
private:
  void create_variable(pas::ast::Type type, PascalIdent name);

  llvm::AllocaInst *codegen_alloc_value_of_type(pas::Type type);
  llvm::Type *get_llvm_type_by_lang_type(TypeKind type);

  // Decl in the name, because may find not only variables,
  //   but different kind of things. TODO: make a common name for them.
  Variable *lookup_decl(const std::string &identifier);

  // llvm related fields
private:
  template <typename T> struct EraseFromParent {
    void operator()(T *ptr) { ptr->eraseFromParent(); }
  };

  llvm::LLVMContext &context_;
  std::unique_ptr<llvm::Module> module_uptr_;

  llvm::Function *current_func_ = nullptr;
  llvm::IRBuilder<> *current_func_builder_ = nullptr;

  // static EraseFromParent<llvm::Function> FunctionDeleter;
  // std::unique_ptr<llvm::Function, decltype(FunctionDeleter)> main_func_uptr_;

  // pascal related fields.
private:
  pas::ScopeStack<Variable, TypeDef> scopes_;

  // Чтобы посмотреть в действии, как работает трансляция, посмотрите видео
  // Андреаса Клинга.
  //   Он делал jit-компилятор javascript в браузере ladybird.
  //   https://www.youtube.com/watch?v=8mxubNQC5O8
};

} // namespace visitor
} // namespace pas
