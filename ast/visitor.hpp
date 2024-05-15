#pragma once

#include <ast/ast.hpp>
#include <ast/utils/get_idx.hpp>
#include <ast/visit.hpp>

#include <iostream>
#include <limits>
#include <unordered_map>

namespace pas {
namespace visitor {

class Printer {
public:
  Printer(std::ostream &stream) : stream_(stream) {}

private:
  MAKE_VISIT_STMT_FRIEND();

  void print_indent() {
    for (size_t i = 0; i < depth_; ++i) {
      stream_ << ' ';
    }
  }

  void visit(pas::ast::ProgramModule &pm) {
    print_indent();
    stream_ << "ProgramModule name=" << pm.program_name_ << '\n';
    depth_ += 1;
    visit(pm.block_);
    depth_ -= 1;
  }
  void visit(pas::ast::Block &block) {
    print_indent();
    stream_ << "Block" << '\n';
    depth_ += 1;
    assert(block.decls_.get() != nullptr);
    visit(*block.decls_.get());
    for (auto &stmt : block.stmt_seq_) {
      visit_stmt(*this, stmt);
    }
    depth_ -= 1;
  }
  void visit(pas::ast::Declarations &decls) {
    print_indent();
    stream_ << "Declarations" << '\n';
    depth_ += 1;
    for (auto &const_def : decls.const_defs_) {
      visit(const_def);
    }
    for (auto &type_def : decls.type_defs_) {
      visit(type_def);
    }
    for (auto &var_decl : decls.var_decls_) {
      visit(var_decl);
    }
    for (auto &subprog_decl : decls.subprog_decls_) {
      visit(subprog_decl);
    }
    depth_ -= 1;
  }
  void visit(pas::ast::ConstDef &const_def) {
    print_indent();
    stream_ << "ConstDef name=" << const_def.ident_ << '\n';
    depth_ += 1;
    visit(const_def.const_expr_);
    depth_ -= 1;
  }
  void visit(pas::ast::TypeDef &type_def) {
    print_indent();
    stream_ << "TypeDef name=" << type_def.ident_ << '\n';
    depth_ += 1;
    visit(type_def.type_);
    depth_ -= 1;
  }
  void visit(pas::ast::VarDecl &var_decl) {
    print_indent();
    stream_ << "VarDecl names=";
    bool first = true;
    for (auto &name : var_decl.ident_list_) {
      if (!first) {
        stream_ << ',';
      }
      stream_ << name;
      first = false;
    }
    stream_ << '\n';
    depth_ += 1;
    visit(var_decl.type_);
    depth_ -= 1;
  }
  void visit(pas::ast::ConstExpr &const_expr) {
    print_indent();
    stream_ << "ConstExpr";
    depth_ += 1;
    if (const_expr.unary_op_.has_value()) {
      switch (const_expr.unary_op_.value()) {
      case pas::ast::UnaryOp::Plus: {
        stream_ << " unary_op=plus";
        break;
      }
      case pas::ast::UnaryOp::Minus: {
        stream_ << " unary_op=minus";
        break;
      }
      default:
        assert(false);
        __builtin_unreachable();
      }
    }
    stream_ << '\n';
    visit(const_expr.factor_);
    depth_ -= 1;
  }
  void visit(pas::ast::ConstFactor &const_factor) {
    print_indent();
    stream_ << "ConstFactor ";
    switch (const_factor.index()) {
    case get_idx(pas::ast::ConstFactorKind::Identifier): {
      stream_ << "identifier " << std::get<std::string>(const_factor) << '\n';
      break;
    }
    case get_idx(pas::ast::ConstFactorKind::Number): {
      stream_ << "number " << std::get<int>(const_factor) << '\n';
      break;
    }
    case get_idx(pas::ast::ConstFactorKind::Bool): {
      stream_ << "bool " << (std::get<bool>(const_factor) ? "true" : "false")
              << '\n';
      break;
    }
    case get_idx(pas::ast::ConstFactorKind::Nil): {
      stream_ << "nil" << '\n';
      break;
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  void visit(pas::ast::Type &node) {}
  void visit(pas::ast::ArrayType &node) {}
  void visit(pas::ast::Subrange &node) {}
  void visit(pas::ast::RecordType &node) {}
  void visit(pas::ast::SetType &node) {}
  void visit(pas::ast::PointerType &node) {}
  void visit(pas::ast::FieldList &node) {}
  void visit(pas::ast::Assignment &assignment) {
    print_indent();
    stream_ << "Assignment" << '\n';
    depth_ += 1;
    visit(assignment.designator_);
    visit(assignment.expr_);
    depth_ -= 1;
  }
  void visit(pas::ast::ProcCall &proc_call) {
    print_indent();
    stream_ << "ProcCall name=" << proc_call.proc_ident_ << '\n';
    depth_ += 1;
    for (pas::ast::Expr &expr : proc_call.params_) {
      visit(expr);
    }
    depth_ -= 1;
  }
  void visit(pas::ast::IfStmt &if_stmt) {
    print_indent();
    stream_ << "IfStmt" << '\n';
    depth_ += 1;
    visit(if_stmt.cond_expr_);
    visit_stmt(*this, if_stmt.then_stmt_);
    if (if_stmt.else_stmt_.has_value()) {
      visit_stmt(*this, if_stmt.else_stmt_.value());
    }
    depth_ -= 1;
  }
  void visit(pas::ast::CaseStmt &node) {}
  void visit(pas::ast::Case &node) {}
  void visit(pas::ast::WhileStmt &while_stmt) {
    print_indent();
    stream_ << "WhileStmt" << '\n';
    depth_ += 1;
    visit(while_stmt.cond_expr_);
    visit_stmt(*this, while_stmt.inner_stmt_);
    depth_ -= 1;
  }
  void visit(pas::ast::RepeatStmt &node) {}
  void visit(pas::ast::ForStmt &for_stmt) {
    print_indent();
    stream_ << "ForStmt ident=" << for_stmt.ident_ << " ";
    switch (for_stmt.dir_) {
    case pas::ast::WhichWay::To: {
      stream_ << "dir=To";
      break;
    }
    case pas::ast::WhichWay::DownTo: {
      stream_ << "dir=DownTo";
      break;
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
    stream_ << '\n';

    depth_ += 1;
    visit(for_stmt.start_val_expr_);
    visit(for_stmt.finish_val_expr_);
    visit_stmt(*this, for_stmt.inner_stmt_);
    depth_ -= 1;
  }
  void visit(pas::ast::Designator &designator) {
    print_indent();
    stream_ << "Designator ident=" << designator.ident_ << '\n';
    depth_ += 1;
    for (pas::ast::DesignatorItem &item : designator.items_) {
      visit(item);
    }
    depth_ -= 1;
  }
  void visit(pas::ast::DesignatorItem &designator_item) {
    switch (designator_item.index()) {
    case get_idx(pas::ast::DesignatorItemKind::ArrayAccess): {
      visit(std::get<pas::ast::DesignatorArrayAccess>(designator_item));
      break;
    }
    case get_idx(pas::ast::DesignatorItemKind::FieldAccess): {
      visit(std::get<pas::ast::DesignatorFieldAccess>(designator_item));
      break;
    }
    case get_idx(pas::ast::DesignatorItemKind::PointerAccess): {
      visit(std::get<pas::ast::DesignatorPointerAccess>(designator_item));
      break;
    }
    }
  }
  void visit(pas::ast::DesignatorArrayAccess &array_access) {
    print_indent();
    stream_ << "DesignatorArrayAccess" << '\n';
    depth_ += 1;
    for (std::unique_ptr<pas::ast::Expr> &expr_ptr : array_access.expr_list_) {
      visit(*expr_ptr);
    }
    depth_ -= 1;
  }
  void visit(pas::ast::DesignatorFieldAccess &field_access) {}
  void visit(pas::ast::DesignatorPointerAccess &pointer_access) {}
  void visit(pas::ast::MemoryStmt &node) {}
  void visit(pas::ast::EmptyStmt &empty_stmt) {}
  void visit(pas::ast::Expr &expr) {
    if (!expr.op_.has_value()) {
      visit(expr.start_expr_);
    } else {
      print_indent();
      stream_ << "Expr op=";
      switch (expr.op_.value().rel) {
      case pas::ast::RelOp::Equal: {
        stream_ << "equal";
        break;
      }
      case pas::ast::RelOp::Greater: {
        stream_ << "greater";
        break;
      }
      case pas::ast::RelOp::GreaterEqual: {
        stream_ << "greater_equal";
        break;
      }
      case pas::ast::RelOp::In: {
        stream_ << "in";
        break;
      }
      case pas::ast::RelOp::Less: {
        stream_ << "less";
        break;
      }
      case pas::ast::RelOp::LessEqual: {
        stream_ << "less_equal";
        break;
      }
      case pas::ast::RelOp::NotEqual: {
        stream_ << "not_equal";
        break;
      }
      default:
        assert(false);
        __builtin_unreachable();
      }

      stream_ << '\n';
      depth_ += 1;
      visit(expr.start_expr_);
      visit(expr.op_.value().expr);
      depth_ -= 1;
    }
  }
  void visit(pas::ast::SimpleExpr &simple_expr) {
    if (simple_expr.ops_.empty() && !simple_expr.unary_op_.has_value()) {
      visit(simple_expr.start_term_);
    } else {
      print_indent();
      stream_ << "SimpleExpr";
      if (simple_expr.unary_op_.has_value()) {
        stream_ << " ";
        switch (simple_expr.unary_op_.value()) {
        case pas::ast::UnaryOp::Plus: {
          stream_ << "unary_op=plus";
          break;
        }
        case pas::ast::UnaryOp::Minus: {
          stream_ << "unary_op=minus";
          break;
        }
        default:
          assert(false);
          __builtin_unreachable();
        }
      }
      stream_ << '\n';

      depth_ += 1;
      visit(simple_expr.start_term_);
      for (pas::ast::SimpleExpr::Op &op : simple_expr.ops_) {
        switch (op.op) {
        case pas::ast::AddOp::Minus: {
          print_indent();
          stream_ << "Minus" << '\n';
          break;
        }
        case pas::ast::AddOp::Plus: {
          print_indent();
          stream_ << "Plus" << '\n';
          break;
        }
        case pas::ast::AddOp::Or: {
          print_indent();
          stream_ << "Or" << '\n';
          break;
        }
        default:
          assert(false);
          __builtin_unreachable();
        }

        visit(op.term);
      }
      depth_ -= 1;
    }
  }
  void visit(pas::ast::Term &term) {
    if (term.ops_.empty()) {
      visit(term.start_factor_);
    } else {
      print_indent();
      stream_ << "Term" << '\n';
      depth_ += 1;
      visit(term.start_factor_);
      for (pas::ast::Term::Op &op : term.ops_) {
        switch (op.op) {
        case pas::ast::MultOp::And: {
          print_indent();
          stream_ << "And" << '\n';
          break;
        }
        case pas::ast::MultOp::IntDiv: {
          print_indent();
          stream_ << "IntDiv" << '\n';
          break;
        }
        case pas::ast::MultOp::Modulo: {
          print_indent();
          stream_ << "Modulo" << '\n';
          break;
        }
        case pas::ast::MultOp::Multiply: {
          print_indent();
          stream_ << "Multiply" << '\n';
          break;
        }
        case pas::ast::MultOp::RealDiv: {
          print_indent();
          stream_ << "RealDiv" << '\n';
          break;
        }
        default:
          assert(false);
          __builtin_unreachable();
        }
        visit(op.factor);
        depth_ -= 1;
      }
    }
  }
  void visit(pas::ast::Factor &factor) {
    switch (factor.index()) {
    case get_idx(pas::ast::FactorKind::Bool): {
      print_indent();
      bool value = std::get<bool>(factor);
      stream_ << "Factor Bool" << (value ? "true" : "false") << '\n';
      break;
    }
    case get_idx(pas::ast::FactorKind::Designator): {
      visit(std::get<pas::ast::Designator>(factor));
      break;
    }
    case get_idx(pas::ast::FactorKind::Expr): {
      visit(*std::get<pas::ast::ExprUP>(factor));
      break;
    }
    case get_idx(pas::ast::FactorKind::Negation): {
      print_indent();
      stream_ << "Factor Not" << '\n';
      visit(std::get<pas::ast::NegationUP>(factor)->factor_);
      break;
    }
    case get_idx(pas::ast::FactorKind::Nil): {
      print_indent();
      stream_ << "Factor Nil" << '\n';
      break;
    }
    case get_idx(pas::ast::FactorKind::Number): {
      print_indent();
      stream_ << "Factor number=" << std::get<int>(factor) << '\n';
      break;
    }
    case get_idx(pas::ast::FactorKind::String): {
      print_indent();
      stream_ << "Factor string=\"" << std::get<std::string>(factor) << "\""
              << '\n';
      break;
    }
    case get_idx(pas::ast::FactorKind::FuncCall): {
      visit(*std::get<pas::ast::FuncCallUP>(factor));
      break;
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  void visit(pas::ast::FuncCall &func_call) {
    print_indent();
    stream_ << "FuncCall ident=" << func_call.func_ident_ << '\n';
    depth_ += 1;
    for (pas::ast::Expr &expr : func_call.params_) {
      visit(expr);
    }
    depth_ -= 1;
  }
  void visit(pas::ast::Element &node) {}
  void visit(pas::ast::SubprogDecl &node) {}
  void visit(pas::ast::ProcDecl &node) {}
  void visit(pas::ast::FuncDecl &node) {}
  void visit(pas::ast::ProcHeading &node) {}
  void visit(pas::ast::FormalParam &node) {}
  void visit(pas::ast::StmtSeq &stmt_seq) {
    print_indent();
    stream_ << "StmtSeq" << '\n';
    depth_ += 1;
    for (pas::ast::Stmt &stmt : stmt_seq.stmts_) {
      visit_stmt(*this, stmt);
    }
    depth_ -= 1;
  }

public:
  void visit(pas::ast::CompilationUnit &cu) { visit(cu.pm_); }

private:
  std::ostream &stream_;
  size_t depth_ = 0;
};

// Must write public, otherwise
//   catch won't catch an exception
//   of our type as std::exception&.
//   https://stackoverflow.com/a/50133665

class DescribedException : public std::exception {
public:
  DescribedException() = delete;

  template <typename T> DescribedException(T msg) : msg_(std::move(msg)) {}

  virtual const char *what() const noexcept override { return msg_.c_str(); }

private:
  std::string msg_;
};

class NotImplementedException : public DescribedException {
public:
  using DescribedException::DescribedException;
};

class SemanticProblemException : public DescribedException {
public:
  using DescribedException::DescribedException;
};

class RuntimeProblemException : public DescribedException {
public:
  using DescribedException::DescribedException;
};

// https://stackoverflow.com/a/25066044
//   Won't work if I have any other functions
//   with name "visit" in child class.
//   Ok, let's change tactics.
// class NotImplementedVisitor {
// public:
// #define FOR_EACH_STMT(stmt_type, stmt_kind)                                    \
//   void visit(stmt_type &stmt) {                                                \
//     throw NotImplementedException("not implemented");                          \
//   }
// #include "ast/utils/enum_stmt.hpp"
// #undef FOR_EACH_STMT
// };

// Примеры IR-а.
//   https://mcyoung.xyz/2023/08/01/llvm-ir/

class Lowerer {
public:
  Lowerer(std::ostream& ir_stream)
    : ir_stream_(ir_stream) {

    // Вкидываем глобальное пространство имен.
    pascal_scopes_.emplace_back();

    // Add unique original names for basic types.
    pascal_scopes_.back()["Integer"] = TypeKind::Integer;
    pascal_scopes_.back()["Char"] = TypeKind::Char;
    pascal_scopes_.back()["String"] = TypeKind::String;

    // Вообще обработку различных типов данных можно
    //   вынести в отдельные файлы. Чтобы менять ir интерфейс
    //   (алгоритмы создания кода, кодирования операций) типа
    //   локально, а не бегая по этому большому файлу.
  }

  void codegen(pas::ast::CompilationUnit &cu) { codegen(cu.pm_); }

private:
  MAKE_VISIT_STMT_FRIEND();

  void codegen(pas::ast::ProgramModule &pm);
  void codegen(pas::ast::Block &block);

  void process_decls(pas::ast::Declarations &decls);

  struct PointerType;
  enum class TypeKind : size_t {
    // Base types
    Integer = 0,
    Char = 1,
    String = 2,

    // Pointer to an existing type
    //        PointerType = 2
  };

  // Always unveiled, synonims are expanded, when type is added to the
  // identifier mapping.
  using Type = std::variant<std::monostate, std::monostate,
                            std::monostate>; //, std::shared_ptr<PointerType>>;
                                             //    struct PointerType {
  //        // Types must be referenced as shared_ptrs:
  //        //   the objects in unordered_map (and plain map, almost any
  //        container)
  //        //   can move around memory, we have to account for that.
  //        // For an unordered map, if it is a hashtable, it happens, then
  //        //   there's too much chance of collision, we have to expand
  //        //   storage so that items are evenly spread.
  //        std::shared_ptr<Type> ref_type;
  //    };

  enum class ValueKind : size_t {
    // Base types
    Integer = 0,
    Char = 1,
    String = 2,

    //          // Pointer to another value.
    //          Pointer = 3
  };

private:
  void visit(pas::ast::MemoryStmt &memory_stmt) {}
  void visit(pas::ast::RepeatStmt &repeat_stmt) {}
  void visit(pas::ast::CaseStmt &case_stmt) {}

  void visit(pas::ast::StmtSeq &stmt_seq) {}

  void visit(pas::ast::IfStmt &if_stmt) {}

  void visit(pas::ast::EmptyStmt &empty_stmt) {}

  void visit(pas::ast::ForStmt &for_stmt) {}

private:
  using IRRegister = std::string;
  using IRFuncName = std::string;
  using PascalIdent = std::string;

  // Храним по идентификатору паскаля регистр, в котором лежит значение.
  //   Это нужно для кодогенерации внутри функции.
  // Причем в IR, по аналогии с ассемблером, нет перекрытия (shadowing,
  //   как -Wshadow), т.к. перед нами не переменные, а регистры. Повторное
  //   указание каких либо действий с регистром влечет перезапись, а не
  //   создание нового регистра. Это и понятно, в IR как таковом нет
  //   областей видимости (scopes), кроме, возможно, функций.
  std::unordered_map<PascalIdent, std::pair<IRRegister, ValueKind>> cg_local_vars_; 
  std::unordered_map<PascalIdent, std::pair<IRRegister, ValueKind>> cg_global_vars_; 

  // Вызовы других функций обрабатываем так: название функции паскаля просто
  //   переделывается в ассемблер (mangling). Или даже вставляется как есть,
  //   или в паскале нет перегрузок.
  // В этой переменной хранятся функции, которые уже можно вызывать,
  //   при кодогенерации некоторой функции. Т.е. ее саму и всех, кто шел в
  //   файле до нее или был объявлен до нее.
  std::vector<std::unordered_map<PascalIdent, IRFuncName>> cg_ready_funcs_; 

  // От прежнего интерпретатора, который вычислял значения, нам нужна только проверка типов.
  //   Ведь весь рецепт вычисления записан в IR как в ассемблере. Причем рецепт один и тот
  //   же для разных значений тех же типов. Но проверка типов в широком смысле: нельзя
  //   прибавлять к типу, должна быть переменная или константа (immediate const, типо 5
  //   и 2 в 5+2); не только то, что в выражениях типо 5+2 с обеих сторон числа.
  // Во время проверки типов рекурсивной производится и сама кодонерегация.
  std::vector<std::unordered_map<PascalIdent, std::variant<TypeKind, ValueKind>>> pascal_scopes_; 

  // Чтобы посмотреть в действии, как работает трансляция, посмотрите видео Андреаса Клинга.
  //   Он делал jit-компилятор javascript в браузере ladybird.
  //   https://www.youtube.com/watch?v=8mxubNQC5O8

  std::ostream& ir_stream_;
};

} // namespace visitor
} // namespace pas
