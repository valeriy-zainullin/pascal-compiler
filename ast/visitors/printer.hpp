#pragma once

#include "ast/ast.hpp"
#include "ast/utils/get_idx.hpp"

#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>

namespace pas {
namespace visitor {

class Printer {
public:
  Printer(std::ostream &stream) : stream_(stream) {}

private:
  std::string get_indent();

  void visit(pas::ast::ProgramModule &pm);
  void visit(pas::ast::Block &block);
  void visit(pas::ast::Declarations &decls);
  void visit(pas::ast::ConstDef &const_def);
  void visit(pas::ast::TypeDef &type_def);
  void visit(pas::ast::VarDecl &var_decl);
  void visit(pas::ast::ConstExpr &const_expr);
  void visit(pas::ast::ConstFactor &const_factor);
  void visit(pas::ast::Type &node);
  void visit(pas::ast::ArrayType &node);
  void visit(pas::ast::Subrange &node);
  void visit(pas::ast::RecordType &node);
  void visit(pas::ast::SetType &node);
  void visit(pas::ast::PointerType &node);
  void visit(pas::ast::FieldList &node);
  void visit(pas::ast::Assignment &assignment);
  void visit(pas::ast::ProcCall &proc_call);
  void visit(pas::ast::StmtSeq &stmt_seq);
  void visit(pas::ast::IfStmt &if_stmt);
  void visit(pas::ast::CaseStmt &node);
  void visit(pas::ast::Case &node);
  void visit(pas::ast::WhileStmt &while_stmt);
  void visit(pas::ast::RepeatStmt &node);
  void visit(pas::ast::ForStmt &for_stmt);
  void visit(pas::ast::Designator &designator);
  void visit(pas::ast::DesignatorItem &designator_item);
  void visit(pas::ast::DesignatorArrayAccess &array_access);
  void visit(pas::ast::DesignatorFieldAccess &field_access);
  void visit(pas::ast::DesignatorPointerAccess &pointer_access);
  void visit(pas::ast::MemoryStmt &node);
  void visit(pas::ast::EmptyStmt &empty_stmt);
  void visit(pas::ast::Stmt &stmt);
  void visit(pas::ast::Expr &expr);
  void visit(pas::ast::SimpleExpr &simple_expr);
  void visit(pas::ast::Term &term);
  void visit(pas::ast::Factor &factor);
  void visit(pas::ast::FuncCall &func_call);
  void visit(pas::ast::Element &node);
  void visit(pas::ast::SubprogDecl &node);
  void visit(pas::ast::ProcDecl &node);
  void visit(pas::ast::FuncDecl &node);
  void visit(pas::ast::ProcHeading &node);
  void visit(pas::ast::FormalParam &node);

public:
  void visit(pas::ast::CompilationUnit &cu);

private:
  std::ostream &stream_;
  size_t depth_ = 0;
};

// https://stackoverflow.com/a/25066044
//   Won't work if I have any other functions
//   with name "visit" in child class.
//   Ok, let's change tactics.
// class NotImplementedVisitor {
// public:
// #define FOR_EACH_STMT(stmt_type, stmt_kind) \
//   void visit(stmt_type &stmt) { \
//     throw NotImplementedException("not implemented"); \
//   }
// #include "ast/utils/enum_stmt.hpp"
// #undef FOR_EACH_STMT
// };

} // namespace visitor
} // namespace pas
