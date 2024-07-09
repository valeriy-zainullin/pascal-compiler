#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::declare_builtin_types() {
  // Add unique original names for basic types.
  auto add_type = [this](BasicType basic_type) {
    return scopes_.store_type({type_to_str(basic_type), basic_type});
  };
  TRY(add_type(BasicType::Integer));
  TRY(add_type(BasicType::Char));
  TRY(add_type(BasicType::String));

  // Don't add a type like string literal, because it's
  //   not a documented language type, but rather a
  //   compiler-specific one, also it's source code name
  //   and logged name is not a valid identifier
  //   "<string literal>".

  return {};
}

} // namespace visitor
} // namespace pas