#pragma once

// In utils folder, because may be reused
//   for all different kinds of ast
//   visitors.

#include <cstddef> // size_t
#include <variant>

#include "llvm/IR/IRBuilder.h"

#include "exceptions.hpp"
#include "support/type.hpp"

namespace pas {
enum class BasicType {
  Integer,
  Boolean,
  Real,
  Char,
  String,
  // ...
};

struct RecordType {
  RecordType() {
    throw pas::NotImplementedException(
        "Record types aren't supported for now.");
  }

  bool operator==(const RecordType &other) {
    // Record types aren't supported for now.
    //   Deem them as different, so that
    //   it's not possible to assign or do
    //   other stuff.
    return false;
  }
};

struct SetType {
  SetType() {
    throw pas::NotImplementedException("Set types aren't supported for now.");
  }

  bool operator==(const ArrayType &other) {
    // Set types aren't supported for now.
    //   Deem them as different, so that
    //   it's not possible to assign or do
    //   other stuff.
    return false;
  }
};

struct ArrayType {
  ArrayType() {
    throw pas::NotImplementedException("Set types aren't supported for now.");
  }

  bool operator==(const ArrayType &other) {
    // Array types aren't supported for now.
    //   Deem them as different, so that
    //   it's not possible to assign or do
    //   other stuff.
    return false;
  }
};

struct PointerType {
  size_t num_ptrs = 0; // Сколько перенаправлений.
  std::variant<BasicType, RecordType, ArrayType, SetType> refd_type;

  bool operator==(const Pointer &other) {
    if (num_ptrs != other.num_ptrs) {
      return false;
    }

      return std::visit(refd_type, [&other)(auto& lhs_type) {
        return std::visit(other, [&lhs_type](const auto& rhs_type) {
      if constexpr (std::is_same_v<decltype(lhs_type), decltype(rhs_type)>) {
        return lhs_type == rhs_type;
      } else {
        // Holding different alternatives -- different types already
        //   (different categories of types).
        return false;
      }
        })
      })
  }
};

// Чего не сделаешь, чтобы forward declaration работал..
//   Нельзя сделать forward declaration для using..
using TypeBase =
    std::variant<BasicType, RecordType, SetType, ArrayType, PointerType>;
class Type : public TypeBase {
  using TypeBase::TypeBase;

  bool operator==(const Type &other) const {
    return std::visit(*this, [&other](const auto &lhs_type) {
      return std::visit(other, [&lhs_type](const auto &rhs_type) {
        if constexpr (std::is_same_v<decltype(lhs_type), decltype(rhs_type)>) {
          return lhs_type == rhs_type;
        } else {
          // Holding different alternatives -- different types already
          //   (different categories of types).
          return false;
        }
      })
    });
  }
}
} // namespace pas
