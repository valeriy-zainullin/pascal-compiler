#include "ast/visitors/lowerer.hpp"

#include "support/error.hpp"

namespace pas {
namespace visitor {

// TODO: delete all these enums with numbers later. First refactor
//   to std::visit everything that uses them.

LowererErrorOr<llvm::Value *> Lowerer::eval(const pas::ast::Expr &expr) {
  llvm::Value *value = TRY(eval(expr.start_expr_));
  if (expr.op_.has_value()) {
    const pas::ast::Expr::Op &op = expr.op_.value();
    llvm::Value *rhs_value = TRY(eval(op.expr));
    switch (op.rel) {
    case pas::ast::RelOp::Equal:
      return ir_builder_->CreateICmpEQ(value, rhs_value);
    case pas::ast::RelOp::GreaterEqual:
      return ir_builder_->CreateICmpSGE(value, rhs_value);
    case pas::ast::RelOp::Greater:
      return ir_builder_->CreateICmpSGT(value, rhs_value);
    case pas::ast::RelOp::LessEqual:
      return ir_builder_->CreateICmpSLE(value, rhs_value);
    case pas::ast::RelOp::Less:
      return ir_builder_->CreateICmpSLT(value, rhs_value);
    case pas::ast::RelOp::NotEqual:
      return ir_builder_->CreateICmpNE(value, rhs_value);
    case pas::ast::RelOp::In: {
      // Dispose "value" here.
      throw NotImplementedException("relation \"in\" is not supported");
    }
    default:
      UNREACHABLE("all cases should be handled.");
    }
  }
  return value;
}

LowererErrorOr<llvm::Value *> Lowerer::eval(const pas::ast::Term &term) {
  llvm::Value *value = TRY(eval(term.start_factor_));
  for (const pas::ast::Term::Op &op : term.ops_) {
    // if (value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    llvm::Value *rhs_value = TRY(eval(op.factor));
    // if (rhs_value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    // Avoid these breaks.
    //   https://stackoverflow.com/a/62603143
    //   I think this inlines. TODO: check that, find some evidence in the
    //   internet.
    value = [&]() {
      switch (op.op) {
      case pas::ast::MultOp::And:
        return ir_builder_->CreateLogicalAnd(value, rhs_value);
      case pas::ast::MultOp::IntDiv:
        return ir_builder_->CreateSDiv(value, rhs_value);
      case pas::ast::MultOp::Modulo:
        return ir_builder_->CreateSRem(value, rhs_value);

      // nsw, nuw and etc.
      //   https://stackoverflow.com/a/61210926
      case pas::ast::MultOp::Multiply:
        return ir_builder_->CreateMul(value, rhs_value);
      case pas::ast::MultOp::RealDiv:
        throw NotImplementedException("real numbers are not supported");

      default:
        UNREACHABLE("all cases should be handled.");
      }
    }();
  }
  return value;
}

LowererErrorOr<llvm::Value *> Lowerer::eval(const pas::ast::Factor &factor) {
  return std::visit(
      [this](const auto &alternative) {
        if constexpr (is_instance_of_v<
                          std::remove_cvref_t<decltype(alternative)>,
                          std::unique_ptr>) {
          // For pas::ast::ExprUP, pas::ast::NegationUP and
          // pas::ast::FuncCallUP.
          //   Implementations of eval for these functions are down below.
          // // eval(const pas::ast::Expr&)
          return eval(*alternative);
        } else {
          return eval(alternative);
        }
      },
      factor);
}

LowererErrorOr<llvm::Value *> Lowerer::eval(bool value) {
  // No dedicated bool type for now for simplicity
  //   (I don't have much time, too many things to do).
  return ir_builder_->getInt1(value);
}

// TODO: make this int32_t in ast, here and in other visitors.
LowererErrorOr<llvm::Value *> Lowerer::eval(int value) {
  return ir_builder_->getInt32(value);
}

// TODO: annotate it is string as a string literal. So return not
//   just llvm::Value*, but TmpValue kind of thing. Make such
//   new type. It will have lang type inside, llvm::Value and
//   string_is_strview, strview len. And make a comment saying
//   it's for string constant support. These constant have to be
//   assigned through a std::string::operator= in __assign_strlitral,
//   indexed just with indexation, also have length stored, so calls to
//   strlen return length of the constant just like a temporary string is
//   created. When a string created out of these constants, length is supplied.
//   Check user can't make a pointer to such a value. Somewhere in evals. And
//   forbid some other places to use these constants, where necessary.
// TODO: make cstrlen, cstrcmp for null-terminated char pointers.
LowererErrorOr<llvm::Value *> Lowerer::eval(std::string value) {
  // TODO: change these to be std::string pointers, that are
  //   manually allocated and deallocated by calling create_str,
  //   dispose_str.
  return ir_builder_->CreateGlobalStringPtr(value);
}

LowererErrorOr<llvm::Value *>
Lowerer::eval([[maybe_unused]] const pas::ast::Nil &value) {
  throw NotImplementedException("Nil is not supported yet");
}

LowererErrorOr<llvm::Value *> Lowerer::eval(const pas::ast::Negation &value) {
  llvm::Value *inner_value = TRY(eval(value.factor_));
  // if (inner_value.index() != 0) {
  //   throw SemanticProblemException(
  //       "Negation is only applicable to integer types and boolean");
  // }

  // TODO: check types!!!
  return ir_builder_->CreateNot(inner_value);
}

LowererErrorOr<llvm::Value *>
Lowerer::eval(const pas::ast::Designator &designator) {
  TRY(scopes_.check_ident_type(designator.ident_, IdentType::Variable));
  auto var = scopes_.find_var(designator.ident_);
  ASSERT(var != nullptr, "check_ident_type above checks identifier is defined "
                         "and it is a variable");

  llvm::Value *base_value = var->memory;

  for ([[maybe_unused]] const pas::ast::DesignatorItem &item :
       designator.items_) {
    throw NotImplementedException(
        "designator element access is not supported for now!");
    //   switch (item.index()) {
    //   case get_idx(pas::ast::DesignatorItemKind::FieldAccess): {
    //     throw NotImplementedException("field access is not implemented");
    //   }
    //   case get_idx(pas::ast::DesignatorItemKind::PointerAccess): {
    //     throw NotImplementedException("pointer access is not implemented");
    //   }
    //   case get_idx(pas::ast::DesignatorItemKind::ArrayAccess): {
    //     //          if (base_value.index() != get_idx(ValueKind::Pointer))
    //     {
    //     //            throw NotImplementedException(
    //     //                "value must be a pointer for array access");
    //     //          }
    //     if (base_value.index() != get_idx(ValueKind::String)) {
    //       throw NotImplementedException(
    //           "value must be a string for array access");
    //     }
    //     pas::ast::DesignatorArrayAccess &array_access =
    //         std::get<pas::ast::DesignatorArrayAccess>(item);
    //     if (array_access.expr_list_.size() != 1) {
    //       throw NotImplementedException(
    //           "array access for more than one index is not supported");
    //     }
    //     Value value_index = eval(*array_access.expr_list_[0]);
    //     if (value_index.index() != get_idx(ValueKind::Integer)) {
    //       throw SemanticProblemException(
    //           "can only do indexing with integer type");
    //     }
    //     int index = std::get<int>(value_index);
    //     auto &value = std::get<std::string>(base_value);
    //     if (index < 0 || index >= value.size()) {
    //       // Won't be reported if it's a compiler, not an interpreter.
    //       //   Only a thing like valgrind or memory sanitizer.
    //       throw RuntimeProblemException("index is out of bounds: " +
    //                                     std::to_string(index));
    //     }
    //     base_value = Value(std::in_place_type<char>, value[index]);
    //     break;
    //   }
    //   }
  }
  // TODO: don't forget to change type, when traversing DesignatorItems above.
  return ir_builder_->CreateLoad(get_llvm_type(var->type), base_value,
                                 designator.ident_);
}

LowererErrorOr<llvm::Value *>
Lowerer::eval(const pas::ast::SimpleExpr &simple_expr) {
  // NOTE: unary op is ignored for now.
  llvm::Value *value = TRY(eval(simple_expr.start_term_));
  for (const pas::ast::SimpleExpr::Op &op : simple_expr.ops_) {
    // if (value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    llvm::Value *rhs_value = TRY(eval(op.term));
    // if (rhs_value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    value = [&]() {
      switch (op.op) {
      case pas::ast::AddOp::Plus:
        return ir_builder_->CreateAdd(value, rhs_value);
      case pas::ast::AddOp::Minus:
        return ir_builder_->CreateSub(value, rhs_value);
      case pas::ast::AddOp::Or:
        return ir_builder_->CreateLogicalOr(value, rhs_value);
      default:
        UNREACHABLE("All cases should be handled!");
      }
    }();
  }
  return value;
}

LowererErrorOr<llvm::Value *>
Lowerer::eval([[maybe_unused]] const pas::ast::FuncCall &func_call) {
  throw pas::NotImplementedException(
      "function calls aren't supported for now.");
}

} // namespace visitor
} // namespace pas
