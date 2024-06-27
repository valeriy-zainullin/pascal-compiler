#include "support/computed_type.hpp"

#include <variant> // std::visit

#include "ast/ast.hpp"
#include "support/scopes.hpp"

// pas::visitor::Lowerer::TypeKind
// Lowerer::make_type_from_ast_type(pas::ast::Type &ast_type) {
//   size_t type_index = ast_type.index();

//   switch (type_index) {
//   case get_idx(pas::ast::TypeKind::Array):
//   case get_idx(pas::ast::TypeKind::Record):
//   case get_idx(pas::ast::TypeKind::Set): {
//     //      throw NotImplementedException(
//     //          "only pointer types, basic types (Integer, Char) and their
//     //          synonims " "are supported for now");
//     throw pas::NotImplementedException(
//         "only basic types (Integer, Char) and strings are supported for
//         now");
//   }
//   case get_idx(pas::ast::TypeKind::Pointer): {
//     throw pas::NotImplementedException("pointer types are not supported
//     now");
//   }

//   //    case get_idx(pas::ast::TypeKind::Pointer): {
//   //      const auto &ptr_type_up = std::get<pas::ast::PointerTypeUP>(type);
//   //      const pas::ast::PointerType &ptr_type = *ptr_type_up;
//   //      if (!ident_to_item_.contains(ptr_type.ref_type_name_)) {
//   //        throw SemanticProblemException(
//   //            "pointer type references an undeclared identifier: " +
//   //            ptr_type.ref_type_name_);
//   //      }
//   //      std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item =
//   //          ident_to_item_[ptr_type.ref_type_name_];
//   //      if (item.index() != 0) {
//   //        throw SemanticProblemException(
//   //            "pointer type must reference a type, not a value: " +
//   //            ptr_type.ref_type_name_);
//   //      }
//   //      auto type_item = std::get<std::shared_ptr<Type>>(item);
//   //      auto pointer_type =
//   //      std::make_shared<PointerType>(PointerType{type_item}); auto
//   //      new_type_item = std::make_shared<Type>(pointer_type); return
//   //      new_type_item; break;
//   //    }
//   case get_idx(pas::ast::TypeKind::Named): {
//     const auto &named_type_up = std::get<pas::ast::NamedTypeUP>(ast_type);
//     const pas::ast::NamedType &named_type = *named_type_up;
//     std::optional<std::variant<TypeKind, Variable>> refd_type;
//     for (auto &scope : pascal_scopes_) {
//       if (scope.contains(named_type.type_name_)) {
//         refd_type = scope[named_type.type_name_];
//       }
//     }

//     if (!refd_type.has_value()) {
//       throw pas::SemanticProblemException(
//           "named type references an undeclared identifier: " +
//           named_type.type_name_);
//     }

//     if (refd_type.value().index() != 0) {
//       throw pas::SemanticProblemException(
//           "named type must reference a type, not a value: " +
//           named_type.type_name_);
//     }

//     auto type = std::get<TypeKind>(refd_type.value());
//     return type;
//   }
//   default: {
//     assert(false);
//     __builtin_unreachable();
//   }
//   }
// }

// using Type = std::variant<SetTypeUP, ArrayTypeUP, PointerTypeUP,
// RecordTypeUP,
//                           NamedTypeUP>;

static pas::SetType
from_ast_type([[maybe_unused]] pas::ScopeStack &scope_stack,
              [[maybe_unused]] const pas::ast::SetType &set_type) {
  throw pas::NotImplementedException("Set types aren't supported for now.");
}

static pas::ArrayType
from_ast_type([[maybe_unused]] pas::ScopeStack &scope_stack,
              [[maybe_unused]] const pas::ast::ArrayType &array_type) {
  throw pas::NotImplementedException("Array types aren't supported for now.");
}

static pas::PointerType
from_ast_type([[maybe_unused]] pas::ScopeStack &scope_stack,
              const pas::ast::PointerType &pointer_type) {
  pas::ComputedType refd_type = scope_stack.find_tdef(pointer_type);
  // TODO: check type was found! If not found, check there's a variable. If so,
  // tell expected type, not variable.
  return std::visit(
      [](auto &inner_type) {
        if constexpr (std::is_same_v<pas::PointerType, decltype(inner_type)>) {
          inner_type.num_ptrs += 1;
          return inner_type;
        } else {
          return pas::PointerType(inner_type);
        }
      },
      refd_type);
}

static pas::RecordType
from_ast_type([[maybe_unused]] pas::ScopeStack &scope_stack,
              [[maybe_unused]] const pas::ast::Record &record_type) {
  throw pas::NotImplementedException("Record types aren't supported for now.");
}

static pas::NamedType
from_ast_type([[maybe_unused]] pas::ScopeStack &scope_stack,
              const pas::ast::NamedType &named_type) {
  // TODO: check type was found! If not found, check there's a variable. If so,
  // tell expected type, not variable.
  // TODO: raise exception in find functions of scope stack, if not found, but
  // there's a different kind of item with such name.
  //   So on correct programs compiler is fast. On incorrect programs, where it
  //   will throw exceptions, it's slow. It's alright.
  // TODO: implement restoration in lowerer, so that if there's an exception,
  // we'll print it and then continue the work. And later on we'll raise the
  // main exception, if there was any exceptions, that some errors were
  // generated. And catch it in main.
  return pas::NamedType(scope_stack.find_tdef(pointer_type));
}

static pas::ComputedType from_ast_type(pas::ScopeStack &scope_stack,
                                       const pas::ast::Type &ast_type) {
  return std::visit(
      [&scope_stack](auto &alternative) {
        return pas::ComputedType(
            from_ast_type(scope_stack, *alternative.get()));
      },
      ast_type);
}

namespace pas {
ComputedType ComputedType::from_ast_type(ScopeStack &scope_stack,
                                         const pas::ast::Type &ast_type) {
  ::from_ast_type(scope_stack, ast_type);
}
} // namespace pas
