#pragma once

#include "ast/ast.hpp"
#include "parser.hh"
#include "parsing/scanner.h"

#include <fstream>
#include <map>
#include <optional>
#include <string>

class Driver {
public:
  Driver();
  std::map<std::string, int> variables;
  int result;
  std::optional<pas::AST> parse(const std::string &f);
  std::string file;

  void scan_begin();
  void scan_end();

  bool trace_parsing;
  bool trace_scanning;
  bool location_debug;

  yy::location location;

  friend class Scanner;
  Scanner scanner;
  yy::parser parser;

private:
  friend yy::parser; // Allow parser to call set_ast.
  void set_ast(pas::AST &&ast);

private:
  std::optional<pas::AST> ast_;
  std::ifstream stream;
};
