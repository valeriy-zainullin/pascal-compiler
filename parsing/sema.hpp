#pragma once

#include "ast.hpp"

namespace pas {
// Semantic checks.
namespace sema {
// Some of operations in grammar are allowed only for specific types.
//   For example, addition is only allowed for integers (maybe for chars also).
//   It's possible that we won't allow addition for chars, we have to convert
//   them to integers first.
// It also checks that all expressions are evaluatable (this also means that
//   all functions called must be defined, not only declared). It's not a must
//   in case we have a linker and functions from other modules. It could be
//   a problem linker should solve. When we start producing object files
//   for ld, we should eliminate this check (functions will come at linking
//   stage and their absence is processed there).
bool typecheck(pas::AST &ast);
} // namespace sema
} // namespace pas
