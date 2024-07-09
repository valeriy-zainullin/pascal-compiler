#pragma once

// TODO: move from exceptions to return values,
//   because most of the time programs have warnings in them,
//   not a good thing to have these as exceptions. Errors
//   though are much more rare, happen only on dev pc's.
//   But not on ci and build machines, at least, much more
//   rarely by design.

// Inspired by llvm::ErrorOr and serenity os ErrorOr class and TRY macro.
//   https://github.com/SerenityOS/serenity/blob/master/AK/Error.h#L144
//   https://github.com/SerenityOS/serenity/blob/master/AK/Try.h#L24
//   https://llvm.org/doxygen/classllvm_1_1ErrorOr.html
// I didn't want to limit my work to only clang, because statement
//   expressions are clang only, as far as I remember.

#include <variant>

#include "support/assert.hpp"

namespace pas {

// How to ignore intentionally.
//   https://stackoverflow.com/questions/53581744/how-can-i-intentionally-discard-a-nodiscard-return-value
//   I'd not cast to void, but rather assign to std::ignore. Looks a bit better
//   in my opinion.
template <typename ErrorType, typename ValueType>
class
    // TODO: uncomment and add below to the specialization for <*, void>.
    // [[nodiscard(
    //   "if error is not handled here, please return it to the parent call "
    //   "(maybe with TRY macro) or explicitly ignore the value."
    // )]]
    ErrorOr {
public:
  static_assert(!std::is_same_v<ErrorType, ValueType>);
  static_assert(!std::is_reference_v<ErrorType>);
  static_assert(!std::is_reference_v<ValueType>);

  // Will bind to anything: temporary objects, references,
  //   the error itself. And we have std::move inside,
  //   we won't have additional copies created.
  //   Error and value type are expected to be both
  //   copy-constructible and move-constructible.

  ErrorOr(ErrorType error) : variant_(std::move(error)) {}
  ErrorOr(ValueType value) : variant_(std::move(value)) {}

  // To construct ErrorOr of a module from an error of a submodule.
  // For example, to construct pas::LowererErrorOr<...> from
  //   pas::ScopeStackError.
  //   Otherwise compiler has to do two user-defined conversions:
  //   pas::ScopeStackError -> pas::LowererError -> pas::LowererErrorOr.
  //   Not gonna happen, standard says only one user-defined constructor
  //   and only one user-defined conversion functions. We have two
  //   user-defined constructors here.
  // This one goes immediately from pas::ScopeStackError -> pas::LowererErrorOr.
  //   And then does the conversion of the first arrow inside.
  template <typename ErrorTypeConvertible>
  ErrorOr(
      ErrorTypeConvertible error_convertible,
      std::enable_if_t<std::is_constructible_v<ErrorType, ErrorTypeConvertible>,
                       std::monostate> = std::monostate())
      : ErrorOr(ErrorType(std::move(error_convertible))) {}

  // The constructor above is tried for llvm::ConstantInt*, if
  //   we have llvm::Value*. llvm::ConstantInt* is implicitly
  //   convertible to llvm::Value*, but because there is a
  //   template, compiler tries to instantiate it and
  //   doesn't perform any implicit conversions.
  // Why do implicit conversions if non-implicit scenario
  //   is possible?
  // Let's write another constructor for implicitly
  //   convertible values. Now for ValueType.
  // These types must be from different hierarchies!
  //   So that both constructors may be applicable
  //   and it'll be a compilation error due to
  //   ambiguity.
  template <typename ValueTypeConvertible>
  ErrorOr(
      ValueTypeConvertible value_convertible,
      std::enable_if_t<std::is_constructible_v<ValueType, ValueTypeConvertible>,
                       std::monostate> = std::monostate())
      : ErrorOr(ValueType(std::move(value_convertible))) {}

  ErrorOr() : variant_(ValueType()) {}
  ErrorOr(const ErrorOr &other) = default;
  ErrorOr(ErrorOr &&other) = default;

  operator bool() { return std::holds_alternative<ValueType>(variant_); }

  ErrorType release_error() {
    // if (variant_.get_index() != 0) {
    //   // Default constructor should be
    //   //   a no error value.
    //   // There should also be bool-conversion
    //   //   operator for the error type, that will
    //   //   tell if it's an error or not.
    //   return ErrorType();
    // }
    ASSERT(std::holds_alternative<ErrorType>(variant_),
           "user must first check if result is an error before extracting an "
           "error!");
    return {std::move(std::get<0>(variant_))};
  }

  ValueType release_value() {
    // std::move doesn't allow copy elision,
    //   but we'll copy contents of error
    //   in the calling code. Let's move
    //   these contents instead, because
    //   Value may surely be expensive
    //   enough to copy, when we can move it.
    ASSERT(std::holds_alternative<ValueType>(variant_),
           "user must first check if result is an error before extracting an "
           "value!");
    return std::move(std::get<1>(variant_));
  }

private:
  std::variant<ErrorType, ValueType> variant_;
};

// Если второго аргумента нет, то просто второй аргумент -- std::monostate.
template <typename ErrorType>
class ErrorOr<ErrorType, void> : public ErrorOr<ErrorType, std::monostate> {
  using ErrorOr<ErrorType, std::monostate>::ErrorOr;
};

} // namespace pas

// GCC extension: statement expressions.
//   Also supported by clang.
//   https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
// Also used by linux kernel.
//   https://stackoverflow.com/a/18885626
//   TODO: find link to file that uses this extesion
//   in linux kernel source code.
// The project also uses -pedantic, so let's ignore
//   the warning for this place specifically. -pedantic
//   is great to catch some bugs. I remember cases in
//   my competitive programming background, when it
//   diagnosed some good bugs.
//   https://gcc.gnu.org/onlinedocs/gcc/Alternate-Keywords.html
// TODO: check if compiles with clang.
// Как работает TRY? Как и в serenityos, сначала вычисляем выражение
//   возвращающее ErrorOr, затем смотрим. Если ошибка (TRY_result
//   контекстуально-приведенный к bool равен false), прокидываем.
//   Иначе выражение, содержащее TRY, вычисляется в значение внутри
//   ErrorOr.
// TODO: найти статью про контекстуальное приведение к bool на хабре.
//   Приведение к булу -- особый случай, это преобразование типов
//   "контекстуальное" для условий внутри if-ов, for-ов и т.п.
// Название переменной не по кодстайлу специально, чтобы не получать
//   перекрытий (shadowing), иначе компилятор будет выдавать ошибку.
#define TRY(expr)                                                              \
  __extension__({                                                              \
    auto TRY_result = (expr);                                                  \
    if (!TRY_result) {                                                         \
      return TRY_result.release_error();                                       \
    }                                                                          \
    TRY_result.release_value();                                                \
  })
