// String io (write_str, read_str).

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::declare_builtin_intio() {
  TRY(declare_func(
      Function{"write_str", {}, {BasicType::String}, false, nullptr}));
  TRY(declare_func(
      Function{"read_str", BasicType::String, {}, false, nullptr}));
}

} // namespace visitor
} // namespace pas
