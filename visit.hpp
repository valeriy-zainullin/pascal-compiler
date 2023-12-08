#pragma once

#include <stmt.hpp>

#include <get_idx.hpp>

#include <cassert>

namespace pas {
namespace ast {

template <typename Visitor, typename Ret = void>
inline Ret visit_stmt(Visitor &visitor, pas::ast::Stmt &stmt) {
  assert(!stmt.valueless_by_exception());
  switch (stmt.index()) {
#define FOR_EACH_STMT(stmt_type, stmt_kind)                              \
  case get_idx(stmt_kind):                                               \
    visitor.visit(*std::get<get_idx(stmt_kind)>(stmt));                  \
    break;
#include <enum_stmt.hpp>
#undef FOR_EACH_STMT
  default:
    assert(false);
    __builtin_unreachable();
  };
}

} // namespace ast
} // namespace pas
