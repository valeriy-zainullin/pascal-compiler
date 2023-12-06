#pragma once

#include <ast.hpp>

namespace pas {
namespace visitor {

class Printer {
private:
  void visit(pas::ast::ProgramModule &node);
  void visit(pas::ast::Block &node);
  void visit(pas::ast::Declarations &node);
  void visit(pas::ast::ConstDef &node);
  void visit(pas::ast::TypeDef &node);
  void visit(pas::ast::VarDecl &node);
  void visit(pas::ast::ConstExpr &node);
  void visit(pas::ast::ConstFactor &node);
  void visit(pas::ast::Type &node);
  void visit(pas::ast::ArrayType &node);
  void visit(pas::ast::Subrange &node);
  void visit(pas::ast::RecordType &node);
  void visit(pas::ast::SetType &node);
  void visit(pas::ast::PointerType &node);
  void visit(pas::ast::FieldList &node);
  void visit(pas::ast::Stmt &node);
  void visit(pas::ast::Assignment &node);
  void visit(pas::ast::ProcCall &node);
  void visit(pas::ast::IfStmt &node);
  void visit(pas::ast::CaseStmt &node);
  void visit(pas::ast::Case &node);
  void visit(pas::ast::WhileStmt &node);
  void visit(pas::ast::RepeatStmt &node);
  void visit(pas::ast::ForStmt &node);
  void visit(pas::ast::WhichWay &node);
  void visit(pas::ast::Designator &node);
  void visit(pas::ast::DesignatorItem &node);
  void visit(pas::ast::MemoryStmt &node);
  void visit(pas::ast::Expr &node);
  void visit(pas::ast::SimpleExpr &node);
  void visit(pas::ast::Term &node);
  void visit(pas::ast::Factor &node);
  void visit(pas::ast::FuncCall &node);
  void visit(pas::ast::Element &node);
  void visit(pas::ast::SubprogDecl &node);
  void visit(pas::ast::ProcDecl &node);
  void visit(pas::ast::FuncDecl &node);
  void visit(pas::ast::ProcHeading &node);
  void visit(pas::ast::FormalParam &node);
  void visit(pas::ast::UnaryOp &node);
  void visit(pas::ast::MultOp &node);
  void visit(pas::ast::AddOp &node);
  void visit(pas::ast::RelOp &node);
public:
  void visit(pas::ast::CompilationUnit &node);
private:
  size_t depth_ = 0;
};

class Interpreter {};

} // namespace visitor
} // namespace pas
