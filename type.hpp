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

using SetTypeUP     = std::unique_ptr<SetType>;
using ArrayTypeUP   = std::unique_ptr<ArrayType>;
using PointerTypeUP = std::unique_ptr<PointerType>;
using RecordTypeUP  = std::unique_ptr<RecordType>;
using NamedTypeUP   = std::unique_ptr<NamedType>;

enum class TypeKind { Set = 0, Array = 1, Pointer = 2, Record = 3, Named = 4 };
using Type = std::variant<SetTypeUP, ArrayTypeUP, PointerTypeUP, RecordTypeUP, NamedTypeUP>;

class Subrange {
    public: Subrange() = default;

  public:
    Subrange(ConstFactor start, ConstFactor finish)
        : start_(std::move(start)), finish_(std::move(finish)) {}

  private:
    ConstFactor start_;
    ConstFactor finish_;
};

class SetType {
    public: SetType() = default;

  public:
    SetType(Subrange subrange) : subrange_(std::move(subrange)) {}

  private:
    Subrange subrange_;
};

class ArrayType {
    public: ArrayType() = default;

  public:
    ArrayType(std::vector<Subrange> subrange_list,
              Type item_type)
        : subrange_list_(std::move(subrange_list)), item_type_(std::move(item_type)) {}

  private:
    std::vector<Subrange> subrange_list_;
    Type item_type_;
};

class PointerType {
    public: PointerType() = default;

  public:
    PointerType(std::string ref_type_name)
        : ref_type_name_(std::move(ref_type_name)) {}

  private:
    std::string ref_type_name_;
};

class FieldList {
  public: FieldList() = default;

public:
  FieldList(std::vector<std::string> idents, Type type) : idents_(std::move(idents)), type_(std::move(type)) {}

private:
  std::vector<std::string> idents_;
  Type type_;
};

class RecordType {
    public: RecordType() = default;

  public:
    RecordType(std::vector<FieldList> field)
        : fields_(std::move(field)) {}

  private:
    std::vector<FieldList> fields_;
};

// An already defined type, which is referenced
//   by a corresponding identifier.
class NamedType {
    public: NamedType() = default;

  public:
    NamedType(std::string type_name) : type_name_(std::move(type_name)) {}

  private:
    std::string type_name_;
};


}
}
