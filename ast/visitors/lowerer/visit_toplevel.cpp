// Visit (functions for) topmost scope

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::visit(const pas::ast::CompilationUnit &cu) {
  return visit(cu.pm_);
}

LowererErrorOr<void> Lowerer::visit(const pas::ast::ProgramModule &pm) {
  return visit_topmost(pm.block_);
}

// TODO: replace topmost with topmost. Toplevel is just parent scope,
//   but topmost is the very first scope!

LowererErrorOr<void>
Lowerer::visit_topmost(const pas::ast::Declarations &decls) {
  // Do what visit_decls does: preform declarations in scope.
  //   But also allow function declarations, which are only possible at
  //   global (topmost) scope.

  if (!decls.const_defs_.empty()) {
    throw pas::NotImplementedException("const defs are not implemented yet");
  }
  for (const auto &type_def : decls.type_defs_) {
    TRY(visit(type_def));
  }
  for (const auto &var_decl : decls.var_decls_) {
    TRY(visit(var_decl));
  }

  // TODO: handle function declarations.
  return {};
}

LowererErrorOr<void> Lowerer::visit_topmost(const pas::ast::Block &block) {
  // Заводим глобальное пространство имен.
  scopes_.push_scope();

  // Определим встроенные типы и встроенные функции.
  declare_builtins();

  // Decl field should always be there, it can just have
  //   no actual decls inside.
  assert(block.decls_.get() != nullptr);

  // These are actually global variables and function decls,
  //   not main function variables. It's what's different
  //   about topmost block in comparison to blocks
  //   of functions.
  visit_topmost(*block.decls_);

  // All subfunctions were generated, let's codegen the main function.

  // declare void @main()
  llvm::FunctionType *main_func_type =
      llvm::FunctionType::get(ir_builder_->getInt32Ty(), false);
  // Create links object to parent. So it's deleted along with the parent.
  //   Won't free it in any specific way. It's a good think to have create.
  //   We didn't allocate with new, so we don't free it with delete. It's
  //   not our responsibility.
  auto main_func =
      llvm::Function::Create(main_func_type, llvm::Function::ExternalLinkage,
                             "main", module_uptr_.get());
  // https://stackoverflow.com/a/10444311

  // entrypoint:
  auto entry = llvm::BasicBlock::Create(context_, "entrypoint", main_func);
  ir_builder_->SetInsertPoint(entry);

  current_func_ = main_func;

  TRY(visit(block.stmt_seq_));

  ir_builder_->CreateRet(ir_builder_->getInt32(0));

  current_func_ = nullptr;
  ir_builder_ = nullptr;

  return {}; // Return some ok value (std::monostate).
}

} // namespace visitor
} // namespace pas