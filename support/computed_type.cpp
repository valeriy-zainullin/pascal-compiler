#include "computed_type.hpp"

namespace pas {

ComputedType PointerType::get_accessed_type() const {
  ASSERT(num_ptrs > 0,
         "В счетчике количества указателей в объявлении типа "
         "считается и сам указатель, т.е. значение не меньше единицы.");
  if (num_ptrs == 1) {
    // Разыменовав, получим безуказательный тип.
    return std::visit(
        [](const auto &refd_type_alt) { return ComputedType(refd_type_alt); },
        refd_type);
  }

  return PointerType{num_ptrs - 1, refd_type};
}

std::string type_to_str(const BasicType &basic_type) {
  switch (basic_type) {
  case BasicType::Integer:
    return "Integer";
  case BasicType::Boolean:
    return "Boolean";
  case BasicType::Real:
    return "Real";
  case BasicType::Char:
    return "Char";
  case BasicType::String:
    return "String";
  case BasicType::StringLiteral:
    return "<string literal>";
  default:
    UNREACHABLE("all basic types should be handled.");
  }
}

std::string type_to_str([[maybe_unused]] const RecordType &record_type) {
  ASSERT(false, "record types aren't supported for now");
  return "?record?";
}

std::string type_to_str([[maybe_unused]] const SetType &set_type) {
  ASSERT(false, "set types aren't supported for now");
  return "?set?";
}

std::string type_to_str([[maybe_unused]] const ArrayType &array_type) {
  ASSERT(false, "array types aren't supported for now");
  return "?array?";
}

std::string type_to_str(const PointerType &pointer_type) {
  return (std::string("^", pointer_type.num_ptrs) +
          std::visit(
              [](const auto &alternative) { return type_to_str(alternative); },
              pointer_type.refd_type));
}

std::string type_to_str(const ComputedType &type) {
  return std::visit(
      [](const auto &alternative) { return type_to_str(alternative); }, type);
}

} // namespace pas
