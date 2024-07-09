// Declaring helpers.

#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::declare_var(std::string name,
                                          pas::ComputedType type) {
  TRY(scopes_.check_ident_type(name, IdentType::NotDefined, true));

  Variable var = {std::move(name), std::move(type), nullptr};

  // Если сейчас активно глобальное пространство имен,
  //   то перед нами глобальная переменная. Выделяем на
  //   уровне всего модуля (единицы трансляции, исходника).
  if (scopes_.is_topmost_scope()) {
    llvm::Type *llvm_type = get_llvm_type(var.type);
    // Создадим глобальную переменную в модуле.
    //   https://stackoverflow.com/a/7787504
    // llvm::Constant::getNullValue - Constructor to create
    //   a '0' constant of arbitrary type.
    //   https://github.com/llvm/llvm-project/blob/3a744283f4c56b57adb2c381c0aeaf7faf5120ec/llvm/include/llvm/IR/Constant.h#L189
    //   https://github.com/llvm/llvm-project/blob/3a744283f4c56b57adb2c381c0aeaf7faf5120ec/llvm/lib/IR/Constants.cpp#L370
    auto llvm_var = new llvm::GlobalVariable(
        *module_uptr_.get(), llvm_type,
        /*isConstant=*/false,
        /*Linkage=*/llvm::GlobalValue::CommonLinkage,
        /*Initializer=*/llvm::Constant::getNullValue(llvm_type));
    module_uptr_->insertGlobalVariable(llvm_var);
    var.memory = llvm_var;
  } else {
    // Выделим память в текущей функции на стеке, затем вставим.
    var.memory = ir_builder_->CreateAlloca(get_llvm_type(var.type));
  }

  // Может не вставиться, а память мы уже выделили. Никакой проблемы нет,
  //   будет ошибка компиляции. А то, что в IR лишнее выделение памяти
  //   (alloca) или в списке глобальных переменных лишняя
  //   -- не страшно.
  TRY(scopes_.store_variable(std::move(var), module_uptr_.get(), ir_builder_));

  return {};
}

LowererErrorOr<void> Lowerer::declare_type(std::string name,
                                           pas::ComputedType type) {
  TRY(scopes_.store_type(
      Type{.name = std::move(name), .type = std::move(type)}));

  return {};
}

LowererErrorOr<void>
Lowerer::declare_func(std::string name,
                      std::optional<pas::ComputedType> ret_type,
                      std::vector<pas::ComputedType> args) {
  // Can't do designated initialization of inherited members.
  //   https://stackoverflow.com/a/72536949
  Function func = {std::move(name), std::move(ret_type), std::move(args)};

  // Функции можно объявлять функции только в глобальной области
  //   видимости, это проверяется в visit для объявления функций.

  llvm::Type *llvm_ret_type = nullptr;
  if (func.ret_type) {
    llvm_ret_type = get_llvm_type(func.ret_type.value());
  } else {
    llvm_ret_type = ir_builder_->getVoidTy();
  }

  std::vector<llvm::Type *> llvm_args;
  for (const ComputedType &pascal_type : func.arg_types) {
    llvm_args.push_back(get_llvm_type(pascal_type));
  }

  // How to declare a function in LLVM and define it later
  //   https://stackoverflow.com/a/22310371

  // TODO: extract external flag from Function, supply here.
  // TODO: implement external flag in the first place.
  llvm::FunctionType *func_type =
      llvm::FunctionType::get(llvm_ret_type, llvm_args, false);
  func.llvm_function =
      llvm::Function::Create(func_type, llvm::Function::ExternalLinkage,
                             func.name, module_uptr_.get());

  // llvm::FunctionType выдалется с помощью placement new в памяти внутри
  //   контекста. Потому освободится вместе с контекстом. А наличие вызова
  //   деструктора санитайзеры видимо не проверяют, т.к. это
  //   библиотека, она уже скомпилирована и проверки туда не вставить.

  // Кладем в стек областей видимости.
  //   Может не вставиться, если не в глобальной
  //   области видимости сейчас или такая функция
  //   уже объявлена. Будет ошибка компиляции. А то,
  //   что в IR лишняя функция (С ТАКИМ ЖЕ ИМЕНЕМ?)
  //   не страшно.
  // TODO: проверить, что будет делать llvm::Function::Create,
  //   если такое имя функции уже использовалось ранее.
  //   Отредактировать коммент выше, убрать капс.
  TRY(scopes_.store_function(std::move(func)));

  return {};
}

} // namespace visitor
} // namespace pas
