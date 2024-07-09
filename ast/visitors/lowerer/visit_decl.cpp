#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::visit(const pas::ast::Declarations &decls) {
  if (!decls.subprog_decls_.empty()) {
    throw pas::SemanticProblemException(
        "function decls are not allowed inside other functions");
  }
  if (!decls.const_defs_.empty()) {
    throw pas::NotImplementedException("const defs are not implemented yet");
  }
  for (auto &type_def : decls.type_defs_) {
    TRY(visit(type_def));
  }
  for (auto &var_decl : decls.var_decls_) {
    TRY(visit(var_decl));
  }

  return {};
}

LowererErrorOr<void> Lowerer::visit(const pas::ast::TypeDef &type_def) {
  pas::ComputedType type = TRY(std::visit(
      [this](const auto &ast_type_alt) {
        return scopes_.compute_ast_type(*ast_type_alt);
      },
      type_def.type_));

  // TODO: rewrite all structure initializations to this style.
  // TODO: replace all bool flags with some special type placeholders
  //       like pas::ScopeStackInterface::OnlyCurrentScope instead of false.
  //       Make two overloads that just interface, if possible.
  //       When I'll do that, explain there, why it's done like this.
  //       So that it is more readable, what this true or false means.
  //       Usually it's possible to understand, what argument means,
  //       by the type of the variable passed into function. Or the object
  //       constructed. And the best case is when all arguments have
  //       different types. Then you know what each argument stands for.
  //       If two arguments have the same type, find a way to unite and
  //       distinguish them. Maybe introduce helper types.
  //       For an operation types should be either obvious like name,
  //       return value and args for function decl. Or they should
  //       be descriptive.
  //       Also, there shouldn't be two arguments with the same type,
  //       they could be rearranged, it's hard to understand what
  //       arguments is what.
  //       This also means that for structure initialization I should
  //       always use designated initializaer (available from C99).
  //       Because for structs almost certainly there are two fields
  //       of the same type. Or will be later. And for funcs I do
  //       what's said before, because there are no named arguments
  //       in C++.
  //       Also mention this there.
  TRY(declare_type(type_def.ident_, type));

  return {};
}

LowererErrorOr<void> Lowerer::visit(const pas::ast::VarDecl &var_decl) {
  pas::ComputedType type = TRY(std::visit(
      [this](const auto &ast_type_alt) {
        return scopes_.compute_ast_type(*ast_type_alt);
      },
      var_decl.type_));

  for (const std::string &ident : var_decl.ident_list_) {
    TRY(declare_var(ident, type));
  }

  return {};
}

} // namespace visitor
} // namespace pas
