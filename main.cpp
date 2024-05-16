#include <fstream>
#include <iostream>
#include <string>

#include "llvm/IR/LLVMContext.h"

#include "ast/visitors/lowerer.hpp"
#include "driver.hh"

int main(int argc, char **argv) {
  int result = 0;
  Driver driver;

  std::string output_path;

  try {
    for (int i = 1; i < argc; ++i) {
      if (argv[i] == std::string("-p")) {
        driver.trace_parsing = true;
      } else if (argv[i] == std::string("-s")) {
        driver.trace_scanning = true;
      } else if (argv[i] == std::string("-l")) {
        driver.location_debug = true;
      } else if (argv[i] == std::string("-o")) {
        i += 1;
        if (i == argc) {
          std::cerr << "Expected output path after -o." << std::endl;
          return 1;
        }
        output_path = std::string(argv[i]);
      } else {
        std::optional<pas::AST> ast = driver.parse(argv[i]);
        if (!ast.has_value()) {
          std::cerr << "Parsing failed for \"" << argv[i] << "\"." << std::endl;
          return 2;
        }

        llvm::LLVMContext context;
        pas::visitor::Lowerer lowerer(context, argv[i], ast.value());

        break;
      }
    }
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << '\n';
    throw;
  }

  return 0;
}
