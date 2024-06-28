#pragma once

#include <cassert>

// Expr should be idempotent, because it may be evaluated twice.
//   But that depends on assume builtin implementation.

#if defined(__clang__)
#define ASSERT(expr, msg)                                                      \
  assert((expr) && msg);                                                       \
  __builtin_assume(expr);
#elif defined(__GNUC__)
#define ASSERT(expr, msg)                                                      \
  assert((expr) && msg);                                                       \
  __builtin_expect(bool(expr), 1);
#elif defined(_MSC_VER)
#define ASSERT(expr, msg)                                                      \
  assert((expr) && msg);                                                       \
  __assume(expr);
#endif
