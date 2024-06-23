#pragma once

// Есть области видимости. Внутри объявления типов и переменных.
// Они разложены на два вектора: вектор ассоциативных массивов типов,
//   вектор ассоциативных массивов переменных. Причем для переменных
//   может быть нужно хранить разную информацию.
// При трансляции в IR для переменных нужно хранить llvm::AllocaInst*,
//   которая обозначает память, которая выделена под нее.
//   А при интерпретации может быть нужно хранить само значение переменной.
// Для типов можно каждый раз заново по языковому типу получать
//   llvm тип, а можно тоже хранить как доп. информацию. Тогда ее
//   создавать будет вызывающая сторона, с помощью своей логики.
// Еще в паскале есть константы, их тоже можно было бы хранить!
//   В отдельном векторе, потом реализую. Здорово.

#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include "ast/type.hpp"

#include "support/computed_type.hpp"

namespace pas {

struct BasicTypeDef {
  std::string name;
  ComputedType type;

  // Derived struct may also store something.
};

struct BasicVariable {
  std::string name;
  ComputedType type;

  // Derived struct may also store something,
  //   for example, llvm::AllocaInst* for
  //   lowerer.

  // Вызывается после создания переменной в области видимости
  //   в функции ScopeStack::create_variable.
  // Можно передать дополнительную информацию. В случае
  //   lowerer, это llvm::IRBuilder, чтобы что-то
  //   скодонерерировать (выделение памяти, создание объекта).
  template <typename... Args> void start_lifetime(Args... args) {}

  // Вызывается при удалении области видимости в связи
  //   с ее покиданием.
  // В ScopeStack::pop_scope.
  // Можно передать дополнительную информацию. В случае
  //   lowerer, это llvm::IRBuilder, чтобы что-то
  //   скодонерерировать (например, удаление объекта, освобождение памяти).
  template <typename... Args> void end_lifetime(Args... args) {}

  // Могут быть еще обработчики для констант и для типов, если понадобится.
};

// TODO: переместить константы до типа, когда они будут реализованы.
//   Отсортируем по "возрастанию константности".
//   Переменные живут во время исполнения программы, могут меняться.
//   Константы живут во время исполнения программы, не могут меняться.
//   Типы вообще не имеют времени жизни (разве что определения типов
//   живут, пока область видимости не закончилась, это да). Существуют
//   при написании, после компиляции стираются и отражаются лишь
//   в сгенерированном коде и в ABI для взаимодействия со
//   сгенерированным кодом..
// Еще хорошо то, что сначала идет более привычное. Компилятор хранит
//   переменные, это ожидаемо. А вот что типы хранит -- мб неочевиднее..
template <typename Variable, typename TypeDef,
          typename Constant = std::monostate>
class ScopeStack {
  static_assert(std::is_base_of_v<BasicVariable, Variable>);
  static_assert(std::is_base_of_v<BasicTypeDef, TypeDef>);
  // static_assert(std::is_base_of_v<BasicConstant, Constant>); // Not
  // implemented for now.

  // Внешние функции.
public:
  void push_scope() {
    scope_vars_.emplace_back();
    scope_tdefs_.emplace_back();
  }

  template <typename... Args> void pop_scope(Args... args) {
    assert(!scope_vars_.empty());
    assert(!scope_tdefs_.empty());

    for (auto &var_entry : scope_vars_) {
      Variable &var = var_entry.second;

      // No perfect forwarding, because each of these
      //   must have it's own copy of args. No moving
      //   involved.
      var.end_lifetime(args...);
    }

    // Also iterate on typedefs and call destroy_typedef, if needed.

    scope_vars_.pop_back();
    scope_tdefs_.pop_back();
  }

  pas::ComputedType compute_ast_type(pas::ast::Type ast_type) {
    return pas::ComputedType(BasicType::Integer);
  }

  template <typename... Args>
  void store_variable(Variable var, Args &&...args) {
    if (find_variable(var.name, true) != nullptr ||
        find_typedef(var.name, true)) {
      throw pas::SemanticProblemException("redefinition of " + var.name);
    }

    // Do something specific that is done
    //   for variable initialization.
    // For example, create underlying
    //   std::string for pascal String
    //   type, if that is your ABI.
    var.start_lifetime(std::forward(args)...);

    scope_vars_.back().insert(var.name, std::move(var));
  }

  void store_typedef(TypeDef tdef) {
    if (find_variable(tdef.name, true) != nullptr ||
        find_typedef(tdef.name, true)) {
      throw pas::SemanticProblemException("redefinition of " + tdef.name);
    }

    // Do something specific that is done
    //   upon typedef construction..
    // ltt_.create_typedef(variable);

    scope_tdefs_.back().insert(tdef.name, std::move(tdef));
  }

  // Implement later.
  /*
  void create_const() {

  }
  */

public:
  Variable *find_variable(std::string name, bool only_current_scope = false) {
    for (auto scope_it = scope_vars_.rbegin(); scope_it != scope_vars_.rend();
         ++scope_it) {
      auto var_it = scope_it->find(name);
      if (var_it != scope_it->end()) {
        return &*var_it;
      }

      if (only_current_scope) {
        break;
      }
    }
    return nullptr;
  }

  Variable *find_typedef(std::string name, bool only_current_scope = false) {
    for (auto scope_it = scope_tdefs_.rbegin(); scope_it != scope_tdefs_.rend();
         ++scope_it) {
      auto tdef_it = scope_it->find(name);
      if (tdef_it != scope_it->end()) {
        return &*tdef_it;
      }

      if (only_current_scope) {
        break;
      }
    }
    return nullptr;
  }

private:
  // Из идентификатора в объявляемый объект.
  using VarsOfScope = std::unordered_map<std::string, Variable>;
  using TypesOfScope = std::unordered_map<std::string, TypeDef>;
  using ConstsOfScope = std::unordered_map<std::string, Constant>;

  // Scopes -- области видимости.
  //   Разложены на три компоненты: переменные, синонимы типов и константы.
  std::vector<VarsOfScope> scope_vars_;
  std::vector<TypesOfScope> scope_tdefs_;
  std::vector<ConstsOfScope> scope_consts_;
};

} // namespace pas
