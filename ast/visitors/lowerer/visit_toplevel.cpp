// Visit (functions for) toplevel scope

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::visit(pas::ast::CompilationUnit &cu) {
  return visit(cu.pm_);
}

LowererErrorOr<void> Lowerer::visit(pas::ast::ProgramModule &pm) {
  return visit_toplevel(pm.block_);
}

LowererErrorOr<void> Lowerer::visit_toplevel(pas::ast::Block &block) {
  // Заводим глобальное пространство имен.
  scopes_.push_scope();

  // Определим встроенные типы и встроенные функции.
  declare_builtins();

  // Decl field should always be there, it can just have
  //   no actual decls inside.
  assert(block.decls_.get() != nullptr);

  if (!block.decls_->subprog_decls_.empty()) {
    throw pas::NotImplementedException(
        "function decls are not implemented yet");
  }
  if (!block.decls_->const_defs_.empty()) {
    throw pas::NotImplementedException("const defs are not implemented yet");
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

  // entrypoint:
  auto entry = llvm::BasicBlock::Create(context_, "entrypoint", main_func);
  main_func_builder.SetInsertPoint(entry);

  current_func_ = main_func;
  ir_builder_ = &main_func_builder;

  for (auto &type_def : block.decls_->type_defs_) {
    TRY(process_type_def(type_def));
  }
  for (auto &var_decl : block.decls_->var_decls_) {
    TRY(process_var_decl(var_decl));
  }

  TRY(visit(block.stmt_seq_));

  ir_builder_->CreateRet(ir_builder_->getInt32(0));

  current_func_ = nullptr;
  ir_builder_ = nullptr;

  return {}; // Return some ok value (std::monostate).
}

} // namespace visitor
} // namespace pas