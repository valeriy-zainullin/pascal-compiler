// Declaring helpers.

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::declare_var(Variable var) {
  TRY(scopes_.check_ident_type(var.name, IdentType::NotDefined, true));

  // Переменная не объявлена. Тогда вставка должна успешно
  //   отработать. Выделим память в IR, затем вставим.
  var.memory = ir_builder_->CreateAlloca(get_llvm_type(var.type));

  [[maybe_unused]] auto result =
      scopes_.store_variable(std::move(var), module_uptr_.get(), ir_builder_);
  ASSERT(result, "Проверили выше, что такого символа еще не "
                 "было; этого должно быть достаточно.");

  return {};
}

LowererErrorOr<void> Lowerer::declare_type(Type type) {
  TRY(scopes_.store_type(std::move(type)));

  return {};
}

LowererErrorOr<void> Lowerer::declare_func(Function func) {
  std::string func_name = func.name;

  // Функции можно объявлять функции только в глобальной области
  //   видимости, это проверяется в visit для объявления функций.
  // Потому эта проверка по факту проверяет, что в глобальном
  //   пространстве имен нет такого же символа.
  TRY(scopes_.check_ident_type(func.name, IdentType::NotDefined));

  // Тогда вставка должна успешно отработать. Объявим в IR,
  //   затем вставим.

  llvm::Type *return_value = nullptr;
  if (func.ret_type) {
    return_value = get_llvm_type(func.ret_type.value());
  } else {
    return_value = ir_builder_->getVoidTy();
  }

  std::vector<llvm::Type *> args;
  for (const ComputedType &pascal_type : func.arg_types) {
    args.push_back(get_llvm_type(pascal_type));
  }

  // How to declare a function in LLVM and define it later
  //   https://stackoverflow.com/a/22310371

  // TODO: extract external flag from Function, supply here.
  // TODO: implement external flag in the first place.
  llvm::FunctionType *func_type =
      llvm::FunctionType::get(return_value, args, false);
  func.llvm_function =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             func.name, module_uptr_.get());

  // llvm::FunctionType выдалется с помощью placement new в памяти внутри
  //   контекста. Потому освободится вместе с контекстом. А наличие вызова
  //   деструктора санитайзеры видимо не проверяют, т.к. это
  //   библиотека, она уже скомпилирована и проверки туда не вставить.

  // Кладем в стек областей видимости.
  [[maybe_unused]] auto result = scopes_.store_function(std::move(func));
  ASSERT(result, "Проверили выше, что такого символа еще не "
                 "было; этого должно быть достаточно.")

  return {};
}

} // namespace visitor
} // namespace pas
