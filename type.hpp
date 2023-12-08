#pragma once

#include <const_expr.hpp>

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace pas {
namespace ast {

class SetType;
class ArrayType;
class PointerType;
class RecordType;
class NamedType;

using SetTypeUP = std::unique_ptr<SetType>;
using ArrayTypeUP = std::unique_ptr<ArrayType>;
using PointerTypeUP = std::unique_ptr<PointerType>;
using RecordTypeUP = std::unique_ptr<RecordType>;
using NamedTypeUP = std::unique_ptr<NamedType>;

enum class TypeKind { Set = 0, Array = 1, Pointer = 2, Record = 3, Named = 4 };
using Type = std::variant<SetTypeUP, ArrayTypeUP, PointerTypeUP, RecordTypeUP,
                          NamedTypeUP>;

class Subrange {
public:
  Subrange() = default;
  Subrange(Subrange &&other) = default;
  Subrange &operator=(Subrange &&other) = default;

public:
  Subrange(ConstFactor start, ConstFactor finish)
      : start_(std::move(start)), finish_(std::move(finish)) {}

public:
  ConstFactor start_;
  ConstFactor finish_;
};

class SetType {
public:
  SetType() = default;
  SetType(SetType &&other) = default;
  SetType &operator=(SetType &&other) = default;

public:
  SetType(Subrange subrange) : subrange_(std::move(subrange)) {}

public:
  Subrange subrange_;
};

class ArrayType {
public:
  ArrayType() = default;
  ArrayType(ArrayType &&other) = default;
  ArrayType &operator=(ArrayType &&other) = default;

public:
  ArrayType(std::vector<Subrange> subrange_list, Type item_type)
      : subrange_list_(std::move(subrange_list)),
        item_type_(std::move(item_type)) {}

public:
  std::vector<Subrange> subrange_list_;
  Type item_type_;
};

class PointerType {
public:
  PointerType() = default;
  PointerType(PointerType &&other) = default;
  PointerType &operator=(PointerType &&other) = default;

public:
  PointerType(std::string ref_type_name)
      : ref_type_name_(std::move(ref_type_name)) {}

public:
  std::string ref_type_name_;
};

class FieldList {
public:
  FieldList() = default;
  FieldList(FieldList &&other) = default;
  FieldList &operator=(FieldList &&other) = default;

public:
  FieldList(std::vector<std::string> idents, Type type)
      : idents_(std::move(idents)), type_(std::move(type)) {}

public:
  std::vector<std::string> idents_;
  Type type_;
};

class RecordType {
public:
  RecordType() = default;
  RecordType(RecordType &&other) = default;
  RecordType &operator=(RecordType &&other) = default;

public:
  RecordType(std::vector<FieldList> field) : fields_(std::move(field)) {}

public:
  std::vector<FieldList> fields_;
};

// An already defined type, which is referenced
//   by a corresponding identifier.
class NamedType {
public:
  NamedType() = default;
  NamedType(NamedType &&other) = default;
  NamedType &operator=(NamedType &&other) = default;

public:
  NamedType(std::string type_name) : type_name_(std::move(type_name)) {}

public:
  std::string type_name_;
};

} // namespace ast
} // namespace pas
