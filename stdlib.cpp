#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/DerivedTypes.h" // llvm::FunctionType

#include <cstdint>
#include <cstdio>

// Explaination why signatures are like this.
//   https://github.com/llvm/llvm-project/blob/dbe63e3d4dc9e4a53c95a6f8fd24c071d0a603e2/llvm/lib/ExecutionEngine/Interpreter/ExternalFunctions.cpp#L104
//   https://github.com/llvm/llvm-project/blob/dbe63e3d4dc9e4a53c95a6f8fd24c071d0a603e2/llvm/lib/ExecutionEngine/Interpreter/ExternalFunctions.cpp#L124
//   https://github.com/llvm/llvm-project/blob/77116bd7d2682dde2bdfc6c4b96d036ffa7bc3b6/llvm/include/llvm/Support/DynamicLibrary.h#L128-L135

// See the link to the interface example.
//   https://github.com/llvm/llvm-project/blob/dbe63e3d4dc9e4a53c95a6f8fd24c071d0a603e2/llvm/lib/ExecutionEngine/Interpreter/ExternalFunctions.cpp#L440
extern "C" llvm::GenericValue
lle_X_write_int(llvm::FunctionType *FT,
                llvm::ArrayRef<llvm::GenericValue> args) {
  llvm::GenericValue GV;

  if (args.size() != 1) {
    fprintf(stderr, "write_int requires exactly one argument!\n");
    GV.IntVal = llvm::APInt(32, 0);
    return GV;
  }

  int result = printf("%d", static_cast<int>(args[0].IntVal.getZExtValue()));
  GV.IntVal = llvm::APInt(32, result);

  if (result < 0) {
    fprintf(stderr, "write_int failed\n");
  }

  return GV;
}
