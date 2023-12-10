#pragma once

#include <fwd_stmt.hpp>

#include <const_expr.hpp>
#include <ops.hpp>

#include <memory>
#include <utility> // std::move
#include <variant>
#include <vector>

namespace pas {
namespace ast {

// Expr is not defined yet, we don't know it's size yet.
//   And variant must allocate members inside of it,
//   not on heap or etc.
//   Like the problem is that we need expr inside expr
//   subvariants.
class Expr;
using ExprUP = std::unique_ptr<Expr>;

class DesignatorFieldAccess {
public:
  DesignatorFieldAccess() = default;
  DesignatorFieldAccess(DesignatorFieldAccess &&other) = default;
  DesignatorFieldAccess &operator=(DesignatorFieldAccess &&other) = default;

public:
  DesignatorFieldAccess(std::string ident) : ident_(std::move(ident)) {}

public:
  std::string ident_;
};
class DesignatorArrayAccess {
public:
  DesignatorArrayAccess() = default;
  DesignatorArrayAccess(DesignatorArrayAccess &&other) = default;
  DesignatorArrayAccess &operator=(DesignatorArrayAccess &&other) = default;

public:
  DesignatorArrayAccess(std::vector<ExprUP> expr_list)
      : expr_list_(std::move(expr_list)) {}

public:
  std::vector<ExprUP> expr_list_;
};
class DesignatorPointerAccess {
public:
  DesignatorPointerAccess() = default;
  DesignatorPointerAccess(DesignatorPointerAccess &&other) = default;
  DesignatorPointerAccess &operator=(DesignatorPointerAccess &&other) = default;
};

enum class DesignatorItemKind {
  FieldAccess = 0,
  ArrayAccess = 1,
  PointerAccess = 2
};

using DesignatorItem =
    std::variant<DesignatorFieldAccess, DesignatorArrayAccess,
                 DesignatorPointerAccess>;

class Designator {
public:
  Designator() = default;
  Designator(Designator &&other) = default;
  Designator &operator=(Designator &&other) = default;

public:
  Designator(std::string ident, std::vector<DesignatorItem> items)
      : ident_(std::move(ident)), items_(std::move(items)) {}

public:
  std::string ident_;
  std::vector<DesignatorItem> items_;
};

enum class FactorKind {
  String = 0,
  Number = 1,
  Bool = 2,
  Nil = 3,
  Designator = 4,
  Expr = 5,
  Negation = 6,
  FuncCall = 7
};

class Negation;
class FuncCall;

using NegationUP = std::unique_ptr<Negation>;
using FuncCallUP = std::unique_ptr<FuncCall>;

using Factor = std::variant<std::string, int, bool, std::monostate, Designator,
                            ExprUP, NegationUP, FuncCallUP>;

class Negation {
public:
  Negation() = default;
  Negation(Negation &&other) = default;
  Negation &operator=(Negation &&other) = default;

public:
  Negation(Factor factor) : factor_(std::move(factor)) {}

public:
  Factor factor_;
};

class Term {
public:
  Term() = default;
  Term(Term &&other) = default;
  Term &operator=(Term &&other) = default;

public:
  struct Op {
    MultOp op;
    Factor factor;
  };
  Term(Factor start_factor, std::vector<Op> ops)
      : start_factor_(std::move(start_factor)), ops_(std::move(ops)) {}

public:
  Factor start_factor_;
  std::vector<Op> ops_;
};

class SimpleExpr {
public:
  SimpleExpr() = default;
  SimpleExpr(SimpleExpr &&other) = default;
  SimpleExpr &operator=(SimpleExpr &&other) = default;

public:
  struct Op {
    AddOp op;
    Term term;
  };
  SimpleExpr(std::optional<UnaryOp> unary_op, Term start_term,
             std::vector<Op> ops)
      : unary_op_(std::move(unary_op)), start_term_(std::move(start_term)),
        ops_(std::move(ops)) {}

public:
  std::optional<UnaryOp> unary_op_;
  Term start_term_;
  std::vector<Op> ops_;
};

class Expr {
public:
  Expr() = default;
  Expr(Expr &&other) = default;
  Expr &operator=(Expr &&other) = default;

public:
  struct Op {
    RelOp rel;
    SimpleExpr expr;
  };
  Expr(SimpleExpr start_expr, std::optional<Op> op = std::optional<Op>())
      : start_expr_(std::move(start_expr)), op_(std::move(op)) {}

public:
  // TODO: refactor rename to simple_expr_. And rename arg in constructor to
  // simple_expr.
  SimpleExpr start_expr_;
  std::optional<Op> op_;
};

// These are allowed only in expressions.
//   This is a kind of an expr, because
//   it returns a value.
// We could allow to return void types,
//   but then we'd need to make expressions
//   also be statements. I.e. expression
//   evaluation should become a statement
//   then.
class FuncCall {
public:
  FuncCall() = default;
  FuncCall(FuncCall &&other) = default;
  FuncCall &operator=(FuncCall &&other) = default;

public:
  FuncCall(std::string func_ident, std::vector<Expr> params)
      : func_ident_(std::move(func_ident)), params_(std::move(params)) {}

public:
  std::string func_ident_;
  std::vector<Expr> params_;
};

} // namespace ast
} // namespace pas
