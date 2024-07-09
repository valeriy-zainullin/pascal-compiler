// String io (write_str, read_str).

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::declare_builtin_strio() {
  TRY(declare_func("write_str", {}, {BasicType::String}));
  TRY(declare_func("read_str", BasicType::String, {}));

  return {};
}

} // namespace visitor
} // namespace pas
