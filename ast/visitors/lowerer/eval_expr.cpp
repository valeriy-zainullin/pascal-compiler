#include "ast/visitors/lowerer.hpp"

#include "support/error.hpp"

namespace pas {
namespace visitor {

// TODO: delete all these enums with numbers later. First refactor
//   to std::visit everything that uses them.

static bool is_int_type(const pas::ComputedType &type) {
  // На данный момент целочисленный тип лишь один -- это Integer.
  //   Потом может появиться LongInt и т.п.
  return type == pas::BasicType::Integer;
}

static bool is_str_type(const pas::ComputedType &type) {
  return type == pas::BasicType::String ||
         type == pas::BasicType::StringLiteral;
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_int(Lowerer::TmpValue lhs,
                                                       pas::ast::RelOp op,
                                                       Lowerer::TmpValue rhs) {
  // На данный момент из целочисленный типов есть один -- Integer (32-ух
  // битный).
  //   Потом может появиться LongInt.

  ASSERT(
      lhs.type == BasicType::Integer && rhs.type == BasicType::Integer,
      "На данный момент из целочисленных типов поддерживается только Integer");

  switch (op) {
  case pas::ast::RelOp::Equal:
    return TmpValue{ir_builder_->CreateICmpEQ(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::NotEqual:
    return TmpValue{ir_builder_->CreateICmpNE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::Less:
    return TmpValue{ir_builder_->CreateICmpSLT(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::Greater:;
    return TmpValue{ir_builder_->CreateICmpSGT(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::LessEqual:
    return TmpValue{ir_builder_->CreateICmpSLE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::GreaterEqual:;
    return TmpValue{ir_builder_->CreateICmpSGE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::In:
    ASSERT(false, "Отношение in обрабатывается вне этой функции.");
  default:
    UNREACHABLE("all cases should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_bool(Lowerer::TmpValue lhs,
                                                        pas::ast::RelOp op,
                                                        Lowerer::TmpValue rhs) {
  ASSERT(lhs.type == BasicType::Boolean && rhs.type == BasicType::Boolean,
         "Функция eval_op_bool может принимать только аргументы типа boolean, "
         "это должна соблюдать вызывающая сторона.");

  switch (op) {
  case pas::ast::RelOp::Equal:
    return TmpValue{ir_builder_->CreateICmpEQ(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::NotEqual:
    return TmpValue{ir_builder_->CreateICmpNE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::Less:
  case pas::ast::RelOp::Greater:
  case pas::ast::RelOp::LessEqual:
  case pas::ast::RelOp::GreaterEqual:
    return TypeError{TypeError::Reason::IncompatibleTypesInCmp,
                     "Only equal and not equal out of relation operators are "
                     "applicable to boolean."};
  case pas::ast::RelOp::In:
    ASSERT(false, "Отношение in обрабатывается вне этой функции.");
  default:
    UNREACHABLE("all cases should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_char(Lowerer::TmpValue lhs,
                                                        pas::ast::RelOp op,
                                                        Lowerer::TmpValue rhs) {
  ASSERT(lhs.type == BasicType::Char && rhs.type == BasicType::Char,
         "Функция eval_op_char может принимать только аргументы типа char, "
         "это должна соблюдать вызывающая сторона.");

  switch (op) {
  case pas::ast::RelOp::Equal:
    return TmpValue{ir_builder_->CreateICmpEQ(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::NotEqual:
    return TmpValue{ir_builder_->CreateICmpNE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::Less:
    return TmpValue{ir_builder_->CreateICmpSLT(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::Greater:
    return TmpValue{ir_builder_->CreateICmpSGT(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::LessEqual:
    return TmpValue{ir_builder_->CreateICmpSLE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::GreaterEqual:
    return TmpValue{ir_builder_->CreateICmpSGE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::In:
    ASSERT(false, "Отношение in обрабатывается вне этой функции.");
  default:
    UNREACHABLE("all cases should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_op_real([[maybe_unused]] Lowerer::TmpValue lhs,
                      [[maybe_unused]] pas::ast::RelOp op,
                      [[maybe_unused]] Lowerer::TmpValue rhs) {
  return NotImplementedError{
      "Типы чисел с плавающей точкой не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_str(Lowerer::TmpValue lhs,
                                                       pas::ast::RelOp op,
                                                       Lowerer::TmpValue rhs) {
  // Есть два строковых типа: String и StringLiteral.
  //   Нужно уметь их сравнивать.

  ASSERT(is_str_type(lhs.type) && is_str_type(rhs.type),
         "eval_op_str ожидает строковые типы в качестве аргументов");

  auto get_str_type_name = [](const pas::ComputedType &type) {
    if (type == BasicType::String) {
      return "str";
    }

    if (type == BasicType::StringLiteral) {
      return "strliteral";
    }

    UNREACHABLE("String and string literal are the "
                "only types allowed to pass to this function.");
  };

  const char *lhs_str_name = get_str_type_name(lhs.type);
  const char *rhs_str_name = get_str_type_name(rhs.type);

  // Плюс -- левоассоциативная операция. Потому достаточно указать тип
  // std::string
  //   самому первому аргументу. Проблема лишь со std::string_view,
  //   который не хочет конкатенироваться со строкой. Потому наверху
  //   создадим const char* из строковых литералов.
  std::string func_name =
      std::string("pas_cmp_") + lhs_str_name + "_" + rhs_str_name;

  llvm::Function *cmp_func = module_uptr_->getFunction(func_name);
  ASSERT(cmp_func != nullptr, "pas_cmp_{str, strliteral}_{str, strliteral} "
                              "should be defined by the declare_builtins.");

  llvm::Value *cmp_value =
      ir_builder_->CreateCall(cmp_func, {lhs.value, rhs.value});

  switch (op) {
  case pas::ast::RelOp::Equal:
    return TmpValue{ir_builder_->CreateICmpEQ(cmp_value, 0),
                    BasicType::Boolean};
  case pas::ast::RelOp::NotEqual:
    return TmpValue{ir_builder_->CreateICmpNE(cmp_value, 0),
                    BasicType::Boolean};
  case pas::ast::RelOp::Less:
    return TmpValue{ir_builder_->CreateICmpSLT(cmp_value, 0),
                    BasicType::Boolean};
  case pas::ast::RelOp::Greater:
    return TmpValue{ir_builder_->CreateICmpSGT(cmp_value, 0),
                    BasicType::Boolean};
  case pas::ast::RelOp::LessEqual:
    return TmpValue{ir_builder_->CreateICmpSLE(cmp_value, 0),
                    BasicType::Boolean};
  case pas::ast::RelOp::GreaterEqual:
    return TmpValue{ir_builder_->CreateICmpSGE(cmp_value, 0),
                    BasicType::Boolean};
  case pas::ast::RelOp::In:
    ASSERT(false, "Отношение in обрабатывается вне этой функции.");
  default:
    UNREACHABLE("all cases should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_op_record([[maybe_unused]] Lowerer::TmpValue lhs,
                        [[maybe_unused]] pas::ast::RelOp op,
                        [[maybe_unused]] Lowerer::TmpValue rhs) {
  return NotImplementedError{
      "Типы записей не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_op_set([[maybe_unused]] Lowerer::TmpValue lhs,
                     [[maybe_unused]] pas::ast::RelOp op,
                     [[maybe_unused]] Lowerer::TmpValue rhs) {
  return NotImplementedError{
      "Типы множеств не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_op_array([[maybe_unused]] Lowerer::TmpValue lhs,
                       [[maybe_unused]] pas::ast::RelOp op,
                       [[maybe_unused]] Lowerer::TmpValue rhs) {
  return NotImplementedError{
      "Типы массивов не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_op_pointer(Lowerer::TmpValue lhs, pas::ast::RelOp op,
                         Lowerer::TmpValue rhs) {
  ASSERT(std::holds_alternative<pas::PointerType>(lhs.type) &&
             std::holds_alternative<pas::PointerType>(rhs.type),
         "Функция eval_op_char может принимать только аргументы типа pointer, "
         "это должна соблюдать вызывающая сторона.")
  ASSERT(lhs.type == rhs.type,
         "Сравнивать можно только указатели одного типа.");

  // How to compare pointers in llvm.
  //   https://stackoverflow.com/a/78508655
  //   https://llvm.org/docs/LangRef.html#icmp-instruction
  //   TLDR: just like integers.

  switch (op) {
  case pas::ast::RelOp::Equal:
    return TmpValue{ir_builder_->CreateICmpEQ(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::NotEqual:
    return TmpValue{ir_builder_->CreateICmpNE(lhs.value, rhs.value),
                    BasicType::Boolean};
  case pas::ast::RelOp::Less:
  case pas::ast::RelOp::Greater:
  case pas::ast::RelOp::LessEqual:
  case pas::ast::RelOp::GreaterEqual:
    return TypeError{TypeError::Reason::IncompatibleTypesInCmp,
                     "Only equal and not equal out of relation operators are "
                     "applicable to pointers."};
  case pas::ast::RelOp::In:
    ASSERT(false, "Отношение in обрабатывается вне этой функции.");
  default:
    UNREACHABLE("all cases should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_int(Lowerer::TmpValue lhs,
                                                       pas::ast::MultOp op,
                                                       Lowerer::TmpValue rhs) {
  ASSERT(
      lhs.type == BasicType::Integer && rhs.type == BasicType::Integer,
      "На данный момент из целочисленных типов поддерживается только Integer");

  switch (op) {
  case pas::ast::MultOp::And:
    // LLVM has just one instruction for bitwise-and and logical-and.
    //   Logical and is just bitwise and applied to integer with
    //   1-bit width.
    //   https://discourse.llvm.org/t/instruction-and-is-or-in-llvm/1414
    return TmpValue{ir_builder_->CreateLogicalAnd(lhs.value, rhs.value),
                    BasicType::Integer};
  case pas::ast::MultOp::IntDiv:
    return TmpValue{ir_builder_->CreateSDiv(lhs.value, rhs.value),
                    BasicType::Integer};
  case pas::ast::MultOp::Modulo:
    return TmpValue{ir_builder_->CreateSRem(lhs.value, rhs.value),
                    BasicType::Integer};
  case pas::ast::MultOp::Multiply:
    // nsw, nuw and etc.
    //   https://stackoverflow.com/a/61210926
    // TODO: elaborate what this comment really means..
    //       I remember I wrote it, then I was choosing what method
    //       to use for multiplication. So mention these methods of
    //       IRBuilder.
    return TmpValue{ir_builder_->CreateMul(lhs.value, rhs.value),
                    BasicType::Integer};
  case pas::ast::MultOp::RealDiv:
    throw NotImplementedException("real numbers are not supported");

  default:
    UNREACHABLE("all multiplication group operations should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_bool(Lowerer::TmpValue lhs,
                                                        pas::ast::MultOp op,
                                                        Lowerer::TmpValue rhs) {
  ASSERT(lhs.type == BasicType::Boolean && rhs.type == BasicType::Boolean,
         "Функция eval_op_bool может принимать только аргументы типа boolean, "
         "это должна соблюдать вызывающая сторона.");

  switch (op) {
  case pas::ast::MultOp::And:
    // LLVM has just one instruction for bitwise-and and logical-and.
    //   Logical and is just bitwise and applied to integer with
    //   1-bit width.
    //   https://discourse.llvm.org/t/instruction-and-is-or-in-llvm/1414
    return TmpValue{ir_builder_->CreateLogicalAnd(lhs.value, rhs.value),
                    BasicType::Integer};

  case pas::ast::MultOp::IntDiv:
  case pas::ast::MultOp::Modulo:
  case pas::ast::MultOp::Multiply:
  case pas::ast::MultOp::RealDiv:
    return TypeError{TypeError::Reason::IncompatibleTypesInMultOp,
                     "Only \"and\" operator out of multiplication group "
                     "operators (and, div, mod, /) is "
                     "applicable to boolean."};

  default:
    UNREACHABLE("all multiplication group operations should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_int(Lowerer::TmpValue lhs,
                                                       pas::ast::AddOp op,
                                                       Lowerer::TmpValue rhs) {
  ASSERT(
      lhs.type == BasicType::Integer && rhs.type == BasicType::Integer,
      "На данный момент из целочисленных типов поддерживается только Integer");

  switch (op) {
  case pas::ast::AddOp::Or:
    // LLVM has just one instruction for bitwise-and and logical-and.
    //   Logical and is just bitwise and applied to integer with
    //   1-bit width.
    //   https://discourse.llvm.org/t/instruction-and-is-or-in-llvm/1414
    // The same applies to or instruction.
    return TmpValue{ir_builder_->CreateLogicalOr(lhs.value, rhs.value),
                    BasicType::Integer};
  case pas::ast::AddOp::Plus:
    return TmpValue{ir_builder_->CreateAdd(lhs.value, rhs.value),
                    BasicType::Integer};
  case pas::ast::AddOp::Minus:
    return TmpValue{ir_builder_->CreateSub(lhs.value, rhs.value),
                    BasicType::Integer};

  default:
    UNREACHABLE("all addition group operations should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_op_bool(Lowerer::TmpValue lhs,
                                                        pas::ast::AddOp op,
                                                        Lowerer::TmpValue rhs) {
  ASSERT(lhs.type == BasicType::Boolean && rhs.type == BasicType::Boolean,
         "Функция eval_op_bool может принимать только аргументы типа boolean, "
         "это должна соблюдать вызывающая сторона.");

  switch (op) {
  case pas::ast::AddOp::Or:
    // LLVM has just one instruction for bitwise-and and logical-and.
    //   Logical and is just bitwise and applied to integer with
    //   1-bit width.
    //   https://discourse.llvm.org/t/instruction-and-is-or-in-llvm/1414
    // The same applies to or instruction.
    return TmpValue{ir_builder_->CreateLogicalOr(lhs.value, rhs.value),
                    BasicType::Boolean};

  case pas::ast::AddOp::Plus:
  case pas::ast::AddOp::Minus:
    return TypeError{TypeError::Reason::IncompatibleTypesInAddOp,
                     "Only \"or\" operator out of addition group operators "
                     "(or, plus, minus) "
                     "is applicable to boolean."};

  default:
    UNREACHABLE("all addition group operations should be handled.");
  }
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval(const pas::ast::Expr &expr) {
  TmpValue value = TRY(eval(expr.start_expr_));
  if (expr.op_.has_value()) {
    const pas::ast::Expr::Op &op = expr.op_.value();
    TmpValue &lhs_value = value;
    TmpValue rhs_value = TRY(eval(op.expr));

    // TODO: handle in relation operator.
    if (op.rel == pas::ast::RelOp::In) {
      // Dispose "value" here.
      throw NotImplementedException("relation \"in\" is not supported");
    }

    // Below are only RelOp::Equal, RelOp::GreaterEqual,
    //   RelOp::Greater, RelOp::LessEqual, RelOp::Less,
    //   RelOp::NotEqual. Comparison operators.

    // Строковые типы можно сравнивать, даже если
    //   по одну сторону строка, а по другую -- строковый литерал.
    if (is_str_type(lhs_value.type) && is_str_type(rhs_value.type)) {
      return eval_op_str(std::move(lhs_value), op.rel, std::move(rhs_value));
    }

    // TODO: реализовать приведение типов.
    //   Вида A2B. Int2Chr и тому подобное.

    // Можно сравнивать числовые типы между собой. Int и LongInt, например.
    if (is_int_type(lhs_value.type) && is_int_type(rhs_value.type)) {
      return eval_op_int(std::move(lhs_value), op.rel, std::move(rhs_value));
    }

    // В остальных случах типы обязаны совпадать.
    if (lhs_value.type != rhs_value.type) {
      return TypeError{TypeError::Reason::IncompatibleTypesInCmp,
                       "left and right hand sides must be of the same type."};
    }

    const auto &type = lhs_value.type;

    if (type == BasicType::Boolean) {
      return eval_op_bool(std::move(lhs_value), op.rel, std::move(rhs_value));
    }
    if (type == BasicType::Char) {
      return eval_op_char(std::move(lhs_value), op.rel, std::move(rhs_value));
    }
    if (type == BasicType::Real) {
      return eval_op_real(std::move(lhs_value), op.rel, std::move(rhs_value));
    }

    return std::visit(
        [this, &lhs_value, &rhs_value,
         &op](const auto &type_alt) -> LowererErrorOr<TmpValue> {
          // Should cover all builtin types, except basic ones, string-alike and
          //   int-alike.
          //   Those are covered before this std::visit.

          // Record types.
          if constexpr (is_same_nocvref_v<decltype(type_alt), RecordType>) {
            return eval_op_record(lhs_value, op.rel, rhs_value);
          }

          // Set types.
          if constexpr (is_same_nocvref_v<decltype(type_alt), SetType>) {
            return eval_op_set(lhs_value, op.rel, rhs_value);
          }

          // Array types.
          if constexpr (is_same_nocvref_v<decltype(type_alt), ArrayType>) {
            return eval_op_array(lhs_value, op.rel, rhs_value);
          }

          // Pointer types.
          if constexpr (is_same_nocvref_v<decltype(type_alt), PointerType>) {
            return eval_op_pointer(lhs_value, op.rel, rhs_value);
          }

          UNREACHABLE("all possible type kinds should be handled");

          return NotImplementedError();
        },
        type);
  }
  return value;
}

// TODO: refactor some code into Typechecker, share code with it.
//   Derive from it, it's a template accepting scope stack type, also
//   having default parameter for scope stack. Write a comment saying
//   why it's done this way. Call typecheck(...) inside the lowerer.
//   pas::ast::visitor::components::Evaluator<concept pas::ast::CodeGenerator>
//   pas::ast::CodeGenerator::Inst, pas::ast::CodeGenerator::AllocaInst
//   codegen_cmp_...()
//   Write a comment saying how it works for Lowerer.
//   Also say what concepts are in general. Now we do the same with templates
//   what was done with pure virtual classes before. Like we'd do interface
//   abstract classes. And derive. Now we can apply concepts and don't do
//   virtual calls, when type of virtual class is known at compile time.
//   pas::ast::visitor::Visitor<concept pas::ast::CodeGenerator> :
//   pas::ast::Evaluator<...>, Does all these visits. Error<T> =
//   std::variant<VisitorError, T::Error>; Returns VisitorErrorOr<T> =
//   std::variant< Typechecker visitor is primarily for ide's to get compilation
//   errors beforehand.
// TODO: make visual studio code plugin for my language.
// TODO: maybe also implement Evaluator, Visitor as part of lowerer.
//   pas::ast::visitor::LowererParts::{Evaluator, Visitor}, derive
//   Visitor from Evaluator.
// So that each module has less code on it's own. More satisfaction
//   with it.

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval(const pas::ast::Term &term) {
  TmpValue value = TRY(eval(term.start_factor_));
  for (const pas::ast::Term::Op &op : term.ops_) {
    auto &lhs_value = value;
    TmpValue rhs_value = TRY(eval(op.factor));

    if (is_int_type(lhs_value.type) && is_int_type(rhs_value.type)) {
      return eval_op_int(lhs_value, op.op, rhs_value);
    }

    if (lhs_value.type == BasicType::Boolean &&
        rhs_value.type == BasicType::Boolean) {
      return eval_op_bool(lhs_value, op.op, rhs_value);
    }

    return TypeError{
        TypeError::Reason::InvalidTypesInMath,
        "left and right hand side types must be both integer kind of type or "
        "both boolean.",
    };
  }

  // Avoid these breaks.
  //   https://stackoverflow.com/a/62603143
  //   I think this inlines. TODO: check that, find some evidence in the
  //   internet.

  return value;
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval(const pas::ast::Factor &factor) {
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

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval(bool value) {
  // Bool is basically 1-bit integer. It's up to llvm how it
  //   implements the one :)
  return TmpValue{ir_builder_->getInt1(value), pas::BasicType::Boolean};
}

// TODO: make this int32_t in ast, here and in other visitors.
LowererErrorOr<Lowerer::TmpValue> Lowerer::eval(int value) {
  // TODO: make a note it's a constant (default value for flag
  //   is false, we pass true here). Allow implicit conversion
  //   of constant to any integer type that can hold this constant.
  return TmpValue{ir_builder_->getInt32(value), pas::BasicType::Integer};
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
LowererErrorOr<Lowerer::TmpValue> Lowerer::eval(std::string value) {
  // TODO: change these to be std::string pointers, that are
  //   manually allocated and deallocated by calling create_str,
  //   dispose_str.
  return TmpValue{
      ir_builder_->CreateGlobalStringPtr(value),
      pas::BasicType::StringLiteral // TODO: store length as part of string
                                    // literal type.
  };
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval([[maybe_unused]] const pas::ast::Nil &value) {
  // TODO: should also say it's immediate constant, like the case
  //   with integers. Implicitly convertible to any pointer in
  //   assignments and comparisons. Integer value for Nil address
  //   is zero.
  throw NotImplementedException("Nil is not supported yet");
}

// Очень хороший пример использования llvm в качестве бекенда
//   компилятора: https://ipc.susu.ru/38277-9.html

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_not_int(TmpValue value) {
  // TODO: accept all integer types here and mention them in the comment.
  //         when there are more than just one integer type.
  ASSERT(value.type == BasicType::Integer,
         "eval_not_int expects argument of type Integer.");

  // TODO: check in practice it's really binary negation.
  return TmpValue{ir_builder_->CreateNot(value.value), value.type};
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_not_bool(TmpValue value) {
  ASSERT(value.type == BasicType::Boolean,
         "eval_not_bool expects argument of type Boolean.");

  // Bool is just Int1, so the same instruction works.

  // TODO: check in practice it's really negation.
  return TmpValue{ir_builder_->CreateNot(value.value), value.type};
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval(const pas::ast::Negation &value) {
  TmpValue inner_value = TRY(eval(value.factor_));

  if (is_int_type(inner_value.type)) {
    return eval_not_int(std::move(inner_value));
  }
  if (inner_value.type == BasicType::Boolean) {
    return eval_not_bool(std::move(inner_value));
  }

  return TypeError{
      TypeError::Reason::InvalidTypesInMath,
      "Not operator is only applicable to integer types and boolean"};
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_access_str(TmpValue value, const pas::ast::DesignatorItem &item) {
  ASSERT(value.type == BasicType::String,
         "eval_access_str expects the argument to be a String.");

  return std::visit(
      [this,
       &value](const auto &item_alt) -> LowererErrorOr<Lowerer::TmpValue> {
        if constexpr (is_same_nocvref_v<decltype(item_alt),
                                        pas::ast::DesignatorFieldAccess>) {
          // TODO: rename DesignatorFieldAccess to DesignatorRecordAccess.
          return AccessError{AccessError::Reason::UnsupportedAccessType,
                             "record access is not possible for type String"};
        }

        if constexpr (is_same_nocvref_v<decltype(item_alt),
                                        pas::ast::DesignatorArrayAccess>) {
          const pas::ast::DesignatorArrayAccess &access = item_alt;

          // Работает, написать.
          // CreateCall pas_str_access
          //   pas_str_create
          //   pas_str_destroy
          //   pas_str_len str_len
          //   pas_str_push str_push
          //   pas_str_drop str_drop
          //   pas_str_assign_strlit string = <string literal>
          //   pas_str_assign_str    string = string
          //   pas_str_access        string[index]

          if (access.expr_list_.size() != 1u) {
            return AccessError{
                AccessError::Reason::WrongNumberOfArrayAccessors,
                "array access for String type must contain only one index!"};
          }

          llvm::Function *func = module_uptr_->getFunction("pas_str_access");
          ASSERT(
              func != nullptr,
              "pas_str_access should be already defined by declare_builtins.");

          std::vector<llvm::Value *> args = {value.value};

          TmpValue index = TRY(eval(*access.expr_list_[0]));
          if (index.type != BasicType::Integer) {
            return TypeError{TypeError::Reason::ArrayAccessorIsNotInteger,
                             "Index type must be an integer"};
          }

          args.push_back(index.value);

          // item_alt.
          // TODO: eval index, check type (should be Integer, LongInt later,
          // allow
          //   implicit conversion to LongInt from Integer).
          // TODO: introduce PtrDiff? And unsigned types.

          llvm::Value *call_result = ir_builder_->CreateCall(
              func, args, "L000_" + func->getName().str());

          return TmpValue{call_result, BasicType::Char};
        }

        if constexpr (is_same_nocvref_v<decltype(item_alt),
                                        pas::ast::DesignatorPointerAccess>) {
          return AccessError{AccessError::Reason::UnsupportedAccessType,
                             "pointer access is not possible for type String"};
        }

        UNREACHABLE("all possible DesignatorItem alternatives "
                    "should be handled already.");
      },
      item);
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_access_record(
    [[maybe_unused]] TmpValue value,
    [[maybe_unused]] const pas::ast::DesignatorItem &item) {
  return NotImplementedError{
      "Типы записей не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_access_set(
    [[maybe_unused]] TmpValue value,
    [[maybe_unused]] const pas::ast::DesignatorItem &item) {
  return NotImplementedError{
      "Типы множеств не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue> Lowerer::eval_access_array(
    [[maybe_unused]] TmpValue value,
    [[maybe_unused]] const pas::ast::DesignatorItem &item) {
  return NotImplementedError{
      "Типы массивов не поддерживаются на данный момент."};
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval_access_pointer(TmpValue value,
                             const pas::ast::DesignatorItem &item) {
  ASSERT(std::holds_alternative<pas::PointerType>(value.type),
         "eval_access_str expects the argument to be a pointer type.");

  return std::visit(
      [this,
       &value](const auto &item_alt) -> LowererErrorOr<Lowerer::TmpValue> {
        if constexpr (is_same_nocvref_v<decltype(item_alt),
                                        pas::ast::DesignatorFieldAccess>) {
          // TODO: rename DesignatorFieldAccess to DesignatorRecordAccess.
          return AccessError{
              AccessError::Reason::UnsupportedAccessType,
              "record access is not possible for type a pointer type like " +
                  type_to_str(value.type)};
        }

        if constexpr (is_same_nocvref_v<decltype(item_alt),
                                        pas::ast::DesignatorArrayAccess>) {
          return AccessError{
              AccessError::Reason::UnsupportedAccessType,
              "array access is not possible for type a pointer type like " +
                  type_to_str(value.type)};
        }

        if constexpr (is_same_nocvref_v<decltype(item_alt),
                                        pas::ast::DesignatorPointerAccess>) {
          PointerType value_type = std::get<pas::PointerType>(value.type);
          ComputedType accessed_type = value_type.get_accessed_type();
          return TmpValue{
              ir_builder_->CreateLoad(
                  get_llvm_type(accessed_type), value.value
                  // TODO: add twines (string explaination to an evaluation).
                  /*,
                  "L000_<expression snippet with spaces replaced to _>" */
                  ),
              accessed_type};
        }

        UNREACHABLE("all possible DesignatorItem alternatives "
                    "should be handled already.");
      },
      item);
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval(const pas::ast::Designator &designator) {
  TRY(scopes_.check_ident_type(designator.ident_, IdentType::Variable));
  auto var = scopes_.find_var(designator.ident_);
  ASSERT(var != nullptr, "check_ident_type above checks identifier is defined "
                         "and it is a variable");

  TmpValue base_value = {var->memory, var->type};

  for (const pas::ast::DesignatorItem &item : designator.items_) {
    if (base_value.type == BasicType::String) {
      base_value = TRY(eval_access_str(std::move(base_value), item));
    }

    base_value = TRY(std::visit(
        [this, &base_value,
         &item](const auto &type_alt) -> LowererErrorOr<Lowerer::TmpValue> {
          // Перечислим все варианты, в которых можно совершить доступ.
          //   Возможно, тип доступа не подходит, но это проверяется внутри
          //   функций.

          if constexpr (is_same_nocvref_v<decltype(type_alt), RecordType>) {
            return eval_access_record(std::move(base_value), item);
          }

          if constexpr (is_same_nocvref_v<decltype(type_alt), SetType>) {
            return eval_access_set(std::move(base_value), item);
            // return NotImplementedError("Set types aren't supported for
            // now.");
          }

          if constexpr (is_same_nocvref_v<decltype(type_alt), ArrayType>) {
            return eval_access_array(std::move(base_value), item);
          }

          if constexpr (is_same_nocvref_v<decltype(type_alt), PointerType>) {
            return eval_access_pointer(std::move(base_value), item);
          }

          // Все остальные случаи запрещены.

          if (std::holds_alternative<pas::ast::DesignatorFieldAccess>(item)) {
            return AccessError(AccessError::Reason::UnsupportedAccessType,
                               "record access is not possible for type " +
                                   type_to_str(type_alt));
          }

          if (std::holds_alternative<pas::ast::DesignatorArrayAccess>(item)) {
            return AccessError(AccessError::Reason::UnsupportedAccessType,
                               "array access is not possible for type " +
                                   type_to_str(type_alt));
          }

          if (std::holds_alternative<pas::ast::DesignatorPointerAccess>(item)) {
            return AccessError(AccessError::Reason::UnsupportedAccessType,
                               "pointer access is not possible for type " +
                                   type_to_str(type_alt));
          }

          UNREACHABLE(
              "All possible designator accessors should be handled above.");

          return AccessError{AccessError::Reason::UnsupportedAccessType, "??"};
        },
        base_value.type));
  }

  return base_value;
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval(const pas::ast::SimpleExpr &simple_expr) {
  // NOTE: unary op is ignored for now.
  TmpValue value = TRY(eval(simple_expr.start_term_));
  for (const pas::ast::SimpleExpr::Op &op : simple_expr.ops_) {
    auto &lhs_value = value;
    TmpValue rhs_value = TRY(eval(op.term));

    if (is_int_type(lhs_value.type) && is_int_type(rhs_value.type)) {
      return eval_op_int(std::move(lhs_value), op.op, std::move(rhs_value));
    }

    if (lhs_value.type == BasicType::Boolean &&
        rhs_value.type == BasicType::Boolean) {
      return eval_op_bool(std::move(lhs_value), op.op, std::move(rhs_value));
    }

    return TypeError{
        TypeError::Reason::InvalidTypesInMath,
        "left and right hand side types must be both integer kind "
        "of type or both boolean.",
    };

    // value = [&]() {
    //   switch (op.op) {
    //   case pas::ast::AddOp::Plus:
    //     return ir_builder_->CreateAdd(value, rhs_value);
    //   case pas::ast::AddOp::Minus:
    //     return ir_builder_->CreateSub(value, rhs_value);
    //   case pas::ast::AddOp::Or:
    //     return ir_builder_->CreateLogicalOr(value, rhs_value);
    //   default:
    //     UNREACHABLE("All cases should be handled!");
    //   }
    // }();
  }
  return value;
}

LowererErrorOr<Lowerer::TmpValue>
Lowerer::eval([[maybe_unused]] const pas::ast::FuncCall &func_call) {
  throw pas::NotImplementedException(
      "function calls aren't supported for now.");
}

} // namespace visitor
} // namespace pas
