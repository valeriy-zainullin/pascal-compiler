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
#include "support/error.hpp"

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

struct ScopeStackError {
  enum class Reason {
    NoError,
    WrongIdentType,
    IdentNotFound,
    Redefinition,
  } reason;
  std::string description;
};
template <typename ValueType>
using ScopeStackErrorOr = pas::ErrorOr<ScopeStackError, ValueType>;
// А в модулях, которые используют наш, мы просто укажем, как
//   преобразоывывать ошибку из ошибка таблицы символов в ошибку
//   модуля: объявим конструктор от такого типа, причем он сможет
//   работать даже неявно. Будет принимать копию и внутри забирать
//   содержимое. И так мы покажем, как преобразовывать ошибки из
//   одного типа в другой тип. Вот бы так на работе было!
// Если вдруг где-то надо вручную переложить, как-то хитро обработать,
//   то пусть тоже делают. Либо в одном месте, либо сделать новый класс
//   ошибки, ее можно будет привести к ошибке модуля. И все.
//   И переиспользовать алгоритм приведения ошибок можно.

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

public:
  enum class IdentType {
    NotDefined,
    Variable,
    TypeDef,
  };

  IdentType find_ident_type(std::string name, bool only_current_scope = false) {
    if (find_variable(name, only_current_scope) != nullptr) {
      return IdentType::Variable;
    }
    if (find_tdef(name, only_current_scope) != nullptr) {
      return IdentType::TypeDef;
    }
    return IdentType::NotDefined;
  }

  std::string_view get_ident_type_desc(IdentType ident_type) {
    switch (ident_type) {
    NotDefined:
      return "not defined identifier";
    Variable:
      return "variable";
    TypeDef:
      return "type definition";
    default:
      UNREACHABLE("all cases should be handled");
    }
  }

  ScopeStackErrorOr<void> check_ident_type(std::string name,
                                           IdentType expected_type,
                                           bool only_current_scope = false) {
    IdentType real_type = find_ident_type(name, only_current_scope);

    if (real_type != expected_type && real_type != IdentType::NotDefined &&
        expected_type != IdentType::NotDefined) {
      return ScopeStackError{ScopeStackError::Reason::WrongIdentType,
                             "expected \"" + name + "\" to be a " +
                                 get_ident_type_desc(expected_type) +
                                 ", but it's a " +
                                 get_ident_type_desc(expected_type) + "!"};
    } else if (real_type != expected_type &&
               real_type == IdentType::NotDefined) {
      return ScopeStackError{ScopeStackError::Reason::IdentNotFound,
                             "identifier " + name +
                                 "wasn't found (expected to be a " +
                                 get_ident_type_desc(expected_type) + ")"};
    } else if (real_type != expected_type &&
               expected_type == IdentType::NotDefined) {
      // Right now the only reason to check for ident type to be not defined is
      // to avoid redefinition.
      //   If that changes, other cases arrive, we'll have to think..
      // Right now assume it's redefinition.
      return ScopeStackError{ScopeStackError::Reason::Redefinition,
                             "redefinition of idenfifier \"" + name + "\""};
    }
  }

public:
  template <typename... Args>
  ScopeStackErrorOr<void> store_variable(Variable var, Args &&...args) {
    if (auto &err =
            check_ident_type(var.name, IdentType::NotDefined).release_error()) {
      return err;
    }

    // Do something specific that is done
    //   for variable initialization.
    // For example, create underlying
    //   std::string for pascal String
    //   type, if that is your ABI.
    var.start_lifetime(std::forward(args)...);

    scope_vars_.back().insert(var.name, std::move(var));
  }

  ScopeStackErrorOr<void> store_typedef(TypeDef tdef) {
    if (auto &err =
            check_ident_type(var.name, IdentType::NotDefined).release_error()) {
      return err;
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
  Variable *find_variable(std::string name, bool only_current_scope = false,
                          bool assert_ident_type = false) {
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
    check_ident_type(name, IdentType::NotDefined);
    if (assert_ident_type &&
        find_typedef(name, only_current_scope) != nullptr) {
      throw pas::SemanticProblemException(
          "expected \"" + name + "\" to be a variable, but it's a type!");
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
    // TODO: make function find_ident_type returning enum, what is behind the
    // identifier.
    //   and check it instead of this.
    //   Also make assert_ident_type(), which would do string formatting on it's
    //   own to reuse in different find_* functions.
    if (assert_ident_type &&
        find_variable(name, only_current_scope) != nullptr) {
      throw pas::SemanticProblemException(
          "expected \"" + name + "\" to be a tpedef, but it's a variable!");
    }
    return nullptr;
  }

public:
  ScopeStackErrorOr<pas::ComputedType>
  compute_ast_type(pas::ast::Type ast_type) {
    return std::visit(
        [this](auto &alternative) {
          return pas::ComputedType(
              from_ast_type(scope_stack, *alternative.get()));
        },
        ast_type);
  }

  ScopeStackErrorOr<pas::ComputedType>
  compute_ast_type(const pas::ast::NamedType &named_type) {
    // TODO: check type was found! If not found, check there's a variable. If
    // so, tell expected type, not variable.
    // TODO: raise exception in find functions of scope stack, if not found, but
    // there's a different kind of item with such name.
    //   So on correct programs compiler is fast. On incorrect programs, where
    //   it will throw exceptions, it's slow. It's alright.
    // TODO: implement restoration in lowerer, so that if there's an exception,
    // we'll print it and then continue the work. And later on we'll raise the
    // main exception, if there was any exceptions, that some errors were
    // generated. And catch it in main.
    TypeDef* tdef = scope_stack.find_tdef(pointer_type));
    if (tdef == nullptr) {
      // If we'd use ErrorOr, we'd just forward the error from the inside and
      // that's it!
      throw pas::SemanticProblemException("")
    }
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
