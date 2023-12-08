#pragma once

#include <ops.hpp>

#include <optional>
#include <string>
#include <variant>

namespace pas {
namespace ast {

enum class ConstFactorKind : size_t {
  Identifier = 0,
  Number = 1,
  Bool = 2,
  Nil = 3
};

using ConstFactor = std::variant<std::string, int, bool, std::monostate>;

class ConstExpr {
public:
  ConstExpr() = default;

  ConstExpr(ConstExpr &&other) = default;
  ConstExpr &operator=(ConstExpr &&other) = default;

public:
  ConstExpr(std::optional<UnaryOp> unary_op, ConstFactor factor)
      : unary_op_(std::move(unary_op)), factor_(std::move(factor)) {}

public:
  std::optional<UnaryOp> unary_op_;
  ConstFactor factor_;
};

// Element:
//   ConstExpr
//   ConstExpr ".." ConstExpr
enum class ElementKind { ConstExpr = 0, ConstExprRange = 1 };
using Element = std::variant<ConstExpr, std::pair<ConstExpr, ConstExpr>>;

} // namespace ast
} // namespace pas
