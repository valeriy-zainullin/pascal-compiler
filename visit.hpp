#pragma once

#include <cassert>

namespace pas {
namespace ast {

// Expr may be not defined yet. So make it a template.
//   It may be not defined, because it can be used
//   in the expr header itself. Same with stmt.

template <typename Visitor, typename Stmt>
inline void visit_stmt(Visitor &visitor, Stmt &stmt) {
  assert(!stmt.valueless_by_exception());
  switch (stmt.index()) {
#define FOR_EACH_EXPR(stmt_type, stmt_kind)                                    \
  case stmt_kind:                                                              \
    std::get<stmt_kind> expr.get<stmt_kind>().accept(visitor);
#include <enum_expr.hpp>
#undef FOR_EACH_EXPR
  default:
    assert(false);
    __builtin_unreachable();
  };
}

} // namespace ast
} // namespace pas
