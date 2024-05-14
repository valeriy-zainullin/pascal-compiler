#pragma once

#include <ast/const_expr.hpp>
#include <ast/decl.hpp>
#include <ast/expr.hpp>
#include <ast/stmt.hpp>

#include <string>
#include <utility> // std::move

namespace pas {
namespace ast {
class ProgramModule {
public:
  ProgramModule() = default;
  ProgramModule(ProgramModule &&other) = default;
  ProgramModule &operator=(ProgramModule &&other) = default;

public:
  ProgramModule(std::string program_name, Block block)
      : program_name_(program_name), block_(std::move(block)) {}

public:
  std::string program_name_;
  Block block_;
};

class CompilationUnit {
public:
  CompilationUnit() = default;
  CompilationUnit(CompilationUnit &&other) = default;
  CompilationUnit &operator=(CompilationUnit &&other) = default;

public:
  CompilationUnit(ProgramModule &&pm) : pm_(std::move(pm)) {}

public:
  ProgramModule pm_;
};

}; // namespace ast

using AST = ast::CompilationUnit;
} // namespace pas
