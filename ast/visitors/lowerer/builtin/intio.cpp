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
  TRY(declare_func("write_int", {}, {BasicType::Integer}));
  // TODO: add another write_int and check what llvm::CreateFunction does,
  //   if a function with such name already exists.
  TRY(declare_func("read_int", BasicType::Integer, {}));

  return {};
}

} // namespace visitor
} // namespace pas

// TODO: also return type! And always return llvm::Value along with it's
//   pascal type.
