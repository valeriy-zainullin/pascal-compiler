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

extern "C" llvm::GenericValue
lle_X_write_str(llvm::FunctionType *FT,
                llvm::ArrayRef<llvm::GenericValue> args) {
  llvm::GenericValue GV;

  if (args.size() != 1) {
    fprintf(stderr, "write_str requires exactly one argument!\n");
    GV.IntVal = llvm::APInt(32, 0);
    return GV;
  }

  int result = printf("%s", reinterpret_cast<char *>(args[0].PointerVal));
  GV.IntVal = llvm::APInt(32, result);

  if (result < 0) {
    fprintf(stderr, "write_str failed\n");
  }

  return GV;
}

extern "C" llvm::GenericValue
lle_X_read_int(llvm::FunctionType *FT,
               llvm::ArrayRef<llvm::GenericValue> args) {
  llvm::GenericValue GV;

  if (args.size() != 0) {
    fprintf(stderr, "read_int accepts no arguments!\n");
    GV.IntVal = llvm::APInt(32, 0);
    return GV;
  }

  int result = 0;
  if (scanf("%d", &result) != 1) {
    fprintf(stderr, "read_int failed\n");
    GV.IntVal = llvm::APInt(32, 0);
    return GV;
  };

  // C++.
  // How to convert preserving bits? Well, if value is negative,
  //   the only conversion available is preserving bits, so that
  //   it's a valid unsigned quantity.
  //   https://stackoverflow.com/a/4975363
  // The same works for unsigned int to int conversion.
  //   https://stackoverflow.com/a/55565186
  // Anyway, it's good to watch out for things like this and check
  //   before doing conversions. Or if a non-value-preserving conversion
  //   is expected, write a comment telling about this.
  GV.IntVal = llvm::APInt(32, static_cast<unsigned int>(result), true);

  return GV;
}
