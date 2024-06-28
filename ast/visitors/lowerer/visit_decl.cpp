LowererErrorOr<void> Lowerer::visit_decls(pas::ast::Declarations &decls) {
  if (!decls.subprog_decls_.empty()) {
    throw pas::SemanticProblemException(
        "function decls are not allowed inside other functions");
  }
  if (!decls.const_defs_.empty()) {
    throw pas::NotImplementedException("const defs are not implemented yet");
  }
  for (auto &type_def : decls.type_defs_) {
    TRY(create_typedef(type_def));
  }
  for (auto &var_decl : decls.var_decls_) {
    TRY(create_var(var_decl));
  }
}

LowererErrorOr<void>
Lowerer::create_type_def([[maybe_unused]] pas::ast::TypeDef &type_def) {}

// llvm::AllocaInst *Lowerer::codegen_alloc_value_of_type(TypeKind type) {
//   return
//   current_func_builder_->CreateAlloca(get_llvm_type_by_lang_type(type));
// }

LowererErrorOr<void> Lowerer::create_var_decl(pas::ast::VarDecl &var_decl) {
  pas::ComputedType type = TRY(scopes_.compute_ast_type(var_decl.type_));
  for (const std::string &ident : var_decl.ident_list_) {
    pascal_scopes_.back()[ident] =
        Variable(codegen_alloc_value_of_type(var_type), std::move(var_type));
  }
}
