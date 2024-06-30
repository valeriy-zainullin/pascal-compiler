#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

void Lowerer::declare_builtin_types() {
  // Add unique original names for basic types.
  scopes_.store_type({"Integer", BasicType::Integer});
  scopes_.store_type({"Char", BasicType::Char});
  scopes_.store_type({"String", BasicType::String});
}

} // namespace visitor
} // namespace pas