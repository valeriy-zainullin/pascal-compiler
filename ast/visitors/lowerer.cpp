#include "ast/visitors/lowerer.hpp"

#include <memory> // std::unique_ptr

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "ast/ast.hpp"
#include "ast/exceptions.hpp"
#include "ast/utils/get_idx.hpp"
#include "ast/visit.hpp"

namespace pas {
namespace visitor {

// auto Lowerer::FunctionDeleter = decltype(Lowerer::FunctionDeleter)();

Lowerer::Lowerer(llvm::LLVMContext &context, const std::string &file_name,
                 pas::ast::CompilationUnit &cu)
    : context_(context) {

  // ; ModuleID = 'top'
  // source_filename = "top"
  module_uptr_ = std::make_unique<llvm::Module>("top", context_);

  // Заводим глобальное пространство имен.
  pascal_scopes_.emplace_back();

  // Add unique original names for basic types.
  pascal_scopes_.back()["Integer"] = TypeKind::Integer;
  pascal_scopes_.back()["Char"] = TypeKind::Char;
  pascal_scopes_.back()["String"] = TypeKind::String;

  // Вообще обработку различных типов данных можно
  //   вынести в отдельные файлы. Чтобы менять ir интерфейс
  //   (алгоритмы создания кода, кодирования операций) типа
  //   локально, а не бегая по этому большому файлу.

  visit(cu);
}

void Lowerer::visit(pas::ast::CompilationUnit &cu) { visit(cu.pm_); }

void Lowerer::visit(pas::ast::ProgramModule &pm) { visit_toplevel(pm.block_); }

void Lowerer::visit_toplevel(pas::ast::Block &block) {
  // Decl field should always be there, it can just have
  //   no actual decls inside.
  assert(block.decls_.get() != nullptr);

  if (!block.decls_->subprog_decls_.empty()) {
    throw pas::ast::NotImplementedException(
        "function decls are not implemented yet");
  }
  if (!block.decls_->const_defs_.empty()) {
    throw pas::ast::NotImplementedException(
        "const defs are not implemented yet");
  }

  // All subfunctions were generated, let's codegen the main function.

  llvm::IRBuilder<> main_func_builder(context_);

  // declare void @main()
  llvm::FunctionType *main_func_type =
      llvm::FunctionType::get(main_func_builder.getInt32Ty(), false);
  // Create links object to parent. So it's deleted along with the parent.
  //   Won't free it in any specific way. It's a good think to have create.
  //   We didn't allocate with new, so we don't free it with delete. It's
  //   not our responsibility.
  auto main_func =
      llvm::Function::Create(main_func_type, llvm::Function::ExternalLinkage,
                             "main", module_uptr_.get());
  // https://stackoverflow.com/a/10444311

  // llvm::FunctionType выдалется с помощью placement new в памяти внутри

  current_func_ = main_func;

  for (auto &type_def : block.decls_->type_defs_) {
    process_type_def(type_def);
  }
  for (auto &var_decl : block.decls_->var_decls_) {
    process_var_decl(var_decl);
  }

  visit(block.stmt_seq_);
}

void Lowerer::visit(pas::ast::StmtSeq &stmt_seq) {
  for (pas::ast::Stmt &stmt : stmt_seq.stmts_) {
    visit_stmt(*this, stmt);
  }
}

void Lowerer::process_decls(pas::ast::Declarations &decls) {
  if (!decls.subprog_decls_.empty()) {
    throw pas::ast::SemanticProblemException(
        "function decls are not allowed inside other functions");
  }
  if (!decls.const_defs_.empty()) {
    throw pas::ast::NotImplementedException(
        "const defs are not implemented yet");
  }
  for (auto &type_def : decls.type_defs_) {
    process_type_def(type_def);
  }
  for (auto &var_decl : decls.var_decls_) {
    process_var_decl(var_decl);
  }
}

void Lowerer::process_type_def(pas::ast::TypeDef &type_def) {}
void Lowerer::process_var_decl(pas::ast::VarDecl &var_decl) {}

void Lowerer::visit(pas::ast::MemoryStmt &memory_stmt) {}
void Lowerer::visit(pas::ast::RepeatStmt &repeat_stmt) {}
void Lowerer::visit(pas::ast::CaseStmt &case_stmt) {}

void Lowerer::visit(pas::ast::IfStmt &if_stmt) {}

void Lowerer::visit(pas::ast::EmptyStmt &empty_stmt) {}

void Lowerer::visit(pas::ast::ForStmt &for_stmt) {}

void Lowerer::visit(pas::ast::Assignment &assignment) {}
void Lowerer::visit(pas::ast::ProcCall &proc_call) {}
void Lowerer::visit(pas::ast::WhileStmt &while_stmt) {}

} // namespace visitor
} // namespace pas
