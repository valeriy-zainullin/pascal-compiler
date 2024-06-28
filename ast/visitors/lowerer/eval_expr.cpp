#include "ast/visitors/lowerer.hpp"

// TODO: delete all these enums with numbers later. First refactor
//   to std::visit everything that uses them.

LowererErrorOR<llvm::Value *> Lowerer::eval(bool value) {
  // No dedicated bool type for now for simplicity
  //   (I don't have much time, too many things to do).
  return current_func_builder_->getInt1(std::get<bool>(factor));
}

// TODO: make this int32_t in ast, here and in other visitors.
LowererErrorOr<llvm::Value *> Lowerer::eval(int value) {
  return current_func_builder_->getInt32(std::get<int>(factor));
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
  return current_func_builder_->CreateGlobalStringPtr(value);
}

LowererErrorOr<llvm::Value *>
Lowerer::eval([[maybe_unused]] const pas::ast::Nil &value) {
  throw NotImplementedException("Nil is not supported yet");
}

LowererErrorOr<llvm::Value *>
Lowerer::eval([[maybe_unused]] const pas::ast::Negation &value) {
  llvm::Value *inner_value = eval(value.factor_);
  // if (inner_value.index() != 0) {
  //   throw SemanticProblemException(
  //       "Negation is only applicable to integer types and boolean");
  // }

  // TODO: check types!!!
  return current_func_builder_->CreateNot(inner_value);
}

LowererErrorOr<llvm::Value *> Lowerer::eval(pas::ast::Designator &value) {
  auto &designator = std::get<pas::ast::Designator>(factor);
  // TODO: check if item with identifier exists in the first place!!
  // IMPORTANT!
  std::variant<TypeKind, Variable> *decl = lookup_decl(designator.ident_);
  if (decl == nullptr) {
    throw SemanticProblemException("declaration not found: " +
                                   designator.ident_);
  }
  if (decl->index() != 1) {
    throw SemanticProblemException(
        "designator must reference a value, not a type: " + designator.ident_);
  }
  llvm::Value *base_value = variable.memory;

  for (pas::ast::DesignatorItem &item : designator.items_) {
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
  return current_func_builder_->CreateLoad(
      get_llvm_type_by_lang_type(variable.type), base_value, designator.ident_);
}

LowererErrorOr<llvm::Value *> Lowerer::eval(const pas::ast::Factor &factor) {
  return std::visit(
      [this](const auto &alternative) {
        if constexpr (std::is_same_v<decltype(alternative), pas::ast::ExprUP>) {
          return eval(*alternative); // eval(const pas::ast::Expr&), above
        } else if (std::is_same_v<decltype(alternative),
                                  pas::ast::NegationUP>) {
          return eval(*alternative); // eval(const pas::ast::Negation&), above
        } else if (std::is_same_v<decltype(alternative),
                                  pas::ast::FuncCallUP>) {
          return eval(*alternative); // eval(const pas::ast::FuncCall&), above
        } else {
          return eval(alternative);
        }
      },
      factor);
}

llvm::Value *Lowerer::eval(pas::ast::Term &term) {
  llvm::Value *value = eval(term.start_factor_);
  for (pas::ast::Term::Op &op : term.ops_) {
    // if (value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    llvm::Value *rhs_value = eval(op.factor);
    // if (rhs_value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    switch (op.op) {
    case pas::ast::MultOp::And: {
      value = current_func_builder_->CreateLogicalAnd(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::IntDiv: {
      value = current_func_builder_->CreateSDiv(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::Modulo: {
      value = current_func_builder_->CreateSRem(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::Multiply: {
      // nsw, nuw and etc.
      //   https://stackoverflow.com/a/61210926
      value = current_func_builder_->CreateMul(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::RealDiv: {
      throw NotImplementedException("real numbers are not supported");
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  return value;
}

llvm::Value *Lowerer::eval(pas::ast::SimpleExpr &simple_expr) {
  // NOTE: unary op is ignored for now.
  llvm::Value *value = eval(simple_expr.start_term_);
  for (pas::ast::SimpleExpr::Op &op : simple_expr.ops_) {
    // if (value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    llvm::Value *rhs_value = eval(op.term);
    // if (rhs_value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    switch (op.op) {
    case pas::ast::AddOp::Plus: {
      value = current_func_builder_->CreateAdd(value, rhs_value);
      break;
    }
    case pas::ast::AddOp::Minus: {
      value = current_func_builder_->CreateSub(value, rhs_value);
      break;
    }
    case pas::ast::AddOp::Or: {
      value = current_func_builder_->CreateLogicalOr(value, rhs_value);
      break;
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  return value;
}

llvm::Value *Lowerer::eval(pas::ast::Expr &expr) {
  llvm::Value *value = eval(expr.start_expr_);
  if (expr.op_.has_value()) {
    pas::ast::Expr::Op &op = expr.op_.value();
    switch (op.rel) {
    case pas::ast::RelOp::Equal: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpEQ(value, rhs_value);
    }
    case pas::ast::RelOp::GreaterEqual: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSGE(value, rhs_value);
    }
    case pas::ast::RelOp::Greater: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSGT(value, rhs_value);
    }
    case pas::ast::RelOp::LessEqual: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSLE(value, rhs_value);
    }
    case pas::ast::RelOp::Less: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSLT(value, rhs_value);
    }
    case pas::ast::RelOp::NotEqual: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpNE(value, rhs_value);
    }
    case pas::ast::RelOp::In: {
      // Dispose "value" here.
      throw NotImplementedException("relation \"in\" is not supported");
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  return value;
}
