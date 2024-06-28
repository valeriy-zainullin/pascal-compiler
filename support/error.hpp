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

template <typename ErrorType, typename ValueType> class ErrorOr {
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
#define TRY(expr)                                                              \
  __extension__({                                                              \
    auto result = (expr);                                                      \
    if (!result) {                                                             \
      return result.release_error();                                           \
    }                                                                          \
    return result.release_value();                                             \
  })
