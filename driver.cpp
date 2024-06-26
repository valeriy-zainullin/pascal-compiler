#include "driver.hh"

#include "parser.hh"
// #include "sema.hpp"
#include "ast/visitors/printer.hpp"

Driver::Driver()
    : trace_parsing(false), trace_scanning(false), location_debug(false),
      scanner(*this), parser(scanner, *this) {
  variables["one"] = 1;
  variables["two"] = 2;
}

void Driver::set_ast(pas::AST &&ast) { ast_.emplace(std::move(ast)); }

std::optional<pas::AST> Driver::parse(const std::string &f) {
  file = f;

  // initialize location positions
  location.initialize(&file);
  scan_begin();
  parser.set_debug_level(trace_parsing);
  if (parser() != 0) {
    std::cerr << "Parsing error!" << std::endl;
    return {};
  }
  scan_end();

  assert(ast_.has_value());

  pas::visitor::Printer printer(std::cerr);
  printer.visit(ast_.value());

  return std::move(ast_);
}

// bool Driver::typecheck() {
//   assert(ast_.has_value());
//   return pas::sema::typecheck(ast_.value());
// }

void Driver::scan_begin() {
  scanner.set_debug(trace_scanning);
  if (file.empty() || file == "-") {
  } else {
    stream.open(file);
    std::cerr << "File name is " << file << std::endl;

    // Restart scanner resetting buffer!
    scanner.yyrestart(&stream);
  }
}

void Driver::scan_end() { stream.close(); }
