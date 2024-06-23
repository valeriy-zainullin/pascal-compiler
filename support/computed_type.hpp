#pragma once

// In utils folder, because may be reused
//   for all different kinds of ast
//   visitors.

#include <cstddef>     // size_t
#include <type_traits> // std::enable_if.
#include <variant>

#include "llvm/IR/IRBuilder.h"

#include "exceptions.hpp"
#include "support/template_utils.hpp"

#include "ast/ast.hpp"

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

  bool operator==([[maybe_unused]] const RecordType &other) const {
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

  bool operator==([[maybe_unused]] const SetType &other) const {
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

  bool operator==([[maybe_unused]] const ArrayType &other) const {
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

  bool operator==(const PointerType &other) const {
    if (num_ptrs != other.num_ptrs) {
      return false;
    }
    return std::visit(
        [&other](const auto &lhs_type) {
          return std::visit(
              [&lhs_type](const auto &rhs_type) {
                if constexpr (std::is_same_v<decltype(lhs_type),
                                             decltype(rhs_type)>) {
                  return lhs_type == rhs_type;
                } else {
                  // Holding different alternatives -- different types already
                  //   (different categories of types).
                  return false;
                }
              },
              other.refd_type);
        },
        refd_type);
  }
};

// Чего не сделаешь, чтобы forward declaration работал..
//   Нельзя сделать forward declaration для using..
using TypeBase =
    std::variant<BasicType, RecordType, SetType, ArrayType, PointerType>;

// expanded type (lang type), not ast type (where we have NamedTypes also)
class ComputedType : public TypeBase {
  using TypeBase::TypeBase;

public:
  bool operator==(const ComputedType &other) const {
    return std::visit(
        [&other](const auto &lhs_type) -> bool {
          return std::visit(
              [&lhs_type](const auto &rhs_type) -> bool {
                if constexpr (std::is_same_v<decltype(lhs_type),
                                             decltype(rhs_type)>) {
                  return lhs_type == rhs_type;
                } else {
                  // Holding different alternatives -- different types already
                  //   (different categories of types).
                  return false;
                }
              },
              other);
        },
        *this);
  }

  // Allow to compare with bare types, not hidden inside ComputedType.
  //   For example, to check a value to have a specific constant type
  //   inside a lowerer or an interpreter.
  // For example, to do computed_type == BasicType::Integer and etc.
  template <typename VariantMember>
  std::enable_if_t<pas::is_variant_member_v<VariantMember, TypeBase>, bool>
  operator==(const VariantMember &other) const {
    return std::visit(
        [&other](const auto &inner_type) -> bool {
          if constexpr (std::is_same_v<decltype(inner_type), decltype(other)>) {
            return inner_type == other;
          } else {
            // Holding different alternatives -- different types already
            //   (different categories of types).
            return false;
          }
        },
        *this);
  }

  static ComputedType from_ast_type(const pas::ast::Type &ast_type);
};

} // namespace pas
