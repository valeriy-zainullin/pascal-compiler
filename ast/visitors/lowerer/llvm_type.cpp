#include "ast/visitors/lowerer.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

#include "support/computed_type.hpp"
#include "support/unreachable.hpp"

static llvm::Type *get_llvm_type(llvm::IRBuilder<> *ir_builder,
                                 const pas::BasicType &basic_type) {
  switch (basic_type) {
  case pas::BasicType::Integer:
    return ir_builder->getInt32Ty();
  case pas::BasicType::Boolean:
    return ir_builder->getInt1Ty();
  // TODO: check real requirements in free pascal. Maybe just float
  //   is not enough, maybe we need double presision float and
  //   llvm float is not double precision. Need to figure this out.
  case pas::BasicType::Real:
    return ir_builder->getFloatTy();
  case pas::BasicType::Char:
    return ir_builder->getInt8Ty();
  case pas::BasicType::String:
    return ir_builder->getInt8Ty()->getPointerTo();

  default:
    UNREACHABLE();
  }
}

static llvm::Type *
get_llvm_type([[maybe_unused]] llvm::IRBuilder<> *ir_builder,
              [[maybe_unused]] const pas::RecordType &record_type) {
  throw pas::NotImplementedException("Record types aren't supported for now.");
}

static llvm::Type *
get_llvm_type([[maybe_unused]] llvm::IRBuilder<> *ir_builder,
              [[maybe_unused]] const pas::SetType &set_type) {
  throw pas::NotImplementedException("Set types aren't supported for now.");
}

static llvm::Type *
get_llvm_type([[maybe_unused]] llvm::IRBuilder<> *ir_builder,
              [[maybe_unused]] const pas::ArrayType &array_type) {
  throw pas::NotImplementedException("Array types aren't supported for now.");
}

static llvm::Type *
get_llvm_type([[maybe_unused]] llvm::IRBuilder<> *ir_builder,
              [[maybe_unused]] const pas::ComputedType &type) {
  // Что сделает std::visit, он внутри проверит, какая альтернатива хранится в
  // std::variant,
  //   для каждой альтернативы вызовет лямбду, которую передали.
  // А наша лямбда "шаблонная" (TODO: найти правильное название в интернете, на
  // русскоязычном
  //   cppreference), она инстанциируется под эти типы.
  // А внутри лямбды вызов перегруженной функции, потому просто вызываем разные
  // функции для
  //   разных альтернатив. Это как switch по типам с получением значения из
  //   std::variant. Только кода меньше.
  return std::visit(
      [&ir_builder](const auto &inner_type) {
        return get_llvm_type(ir_builder, inner_type);
      },
      type);
}

namespace pas {
namespace visitor {

llvm::Type *
Lowerer::get_llvm_type([[maybe_unused]] const pas::ComputedType &type) {
  // Qualify global namespace with ::, because argument-dependent lookup
  //   won't find it. Namespace is the list of associated namespaces.
  //   Associated namespaces here: llvm::, ir builder is there;
  //   pas::, because of the second argument.
  // It will also try to find suitable method in pas::visitor::Lowerer,
  //   because we're implementing a method of that class. But that won't
  //   succeed. So the compiler doesn't know it has to move some namespaces
  //   up from pas::visitor::Lowerer. Also, that is a language requirement
  //   probably, because ADL is a mechanism of finding the right function.
  return ::get_llvm_type(current_func_builder_, type);
}

} // namespace visitor
} // namespace pas
