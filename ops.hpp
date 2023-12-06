#pragma once

namespace pas {
namespace ast {

enum class AddOp { Plus, Minus, Or };
enum class UnaryOp { Plus, Minus };
enum class MultOp { Multiply, RealDiv, IntDiv, Modulo, And };
enum class RelOp {
  Equal,
  NotEqual,
  Less,
  Greater,
  LessEqual,
  GreaterEqual,
  In
};

} // namespace ast
} // namespace pas
