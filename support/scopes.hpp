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

#include <variant>

struct BasicLifeTimeTracker {
  // Вызывается после создания переменной в области видимости
  //   в функции ScopeStack::create_variable.
  template <typename Variable> void create_variable(Variable &variable) {}

  // Вызывается при удалении области видимости в связи
  //   с ее покиданием.
  // В ScopeStack::pop_scope.
  template <typename Variable> void destroy_variable(Variable &variable) {}

  // Могут быть еще обработчики для констант и для типов, если понадобится.
};

template <typename TypeInfo, typename VarInfo,
          typename ConstInfo = std::monostate, typename LifeTimeTracker>
class ScopeStack {
  // Полезные типы.
public:
  // VarInfo = llvm::AllocaInst* for lowerer.
  // Could be a value variant for self-written visiting interpreter.
  struct Variable : public VarInfo {
    pas::Type type; // expanded type (lang type), not ast type (where we have
                    // NamedTypes also)
  };

  // TypeInfo = llvm::Type* for lowerer
  // Could be std::monostate for self-written visiting interpreter.
  struct TypeAlias : public TypeInfo {
    pas::Type type; // expanded type (lang type)
  }

  struct Constant : public ConstInfo {
    // pas::ConstType type;
  }

  // Внешние функции.
  public : void
           push_scope() {
  }

  void pop_scope() {}

  pas::Type expand_ast_type(pas::ast::Type ast_type) {}

  void create_variable() {}

  void create_typedef() {}

  // Implement later.
  /*
  void create_const() {

  }
  */
private:
  // Из идентификатора в объявляемый объект.
  using VarsOfScope = std::unordered_set<std::string, Variable>;
  using TypesOfScope = std::unordered_set<std::string, TypeAlias>;
  using ConstsOfScope = std::unordered_set<std::string, Constant>;

  // LifeTimeTracker выполняет различные действия при создании или удалении
  // переменной.
  //   Например, для строк может создавать и освобождать std::string, который
  //   находится по указателю.
  LifeTimeTracker ltt_;

  // Scopes -- области видимости.
  //   Разложены на три компоненты: переменные, синонимы типов и константы.
  std::vector<VarsOfScope> scope_vars_;
  std::vector<TypesOfScope> scope_types_;
  std::vector<ConstsOfScope> scope_consts_;
};
