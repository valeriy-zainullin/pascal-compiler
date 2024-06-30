// Int io (write_int, read_int).

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::declare_builtin_intio() {
  // These are not forward declarations, but rather external functions.
  //   https://www.freepascal.org/docs-html/ref/refse98.html
  // TODO: make a bool field in BasicFunction for externality of function.
  //   Also make sanity check function sanity_check(), it checks that
  //   function external or it's a forward declaration, but not both at the same
  //   time. Also, libraries may be mentioned for these functions.
  TRY(declare_func(
      Function{"write_int", {}, {BasicType::Integer}, false, nullptr}));
  TRY(declare_func(
      Function{"read_int", BasicType::Integer, {}, false, nullptr}));
}

} // namespace visitor
} // namespace pas

// TODO: also return type! And always return llvm::Value along with it's
//   pascal type.
