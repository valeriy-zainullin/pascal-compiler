#include "printer.hpp"

#include <cassert>
#include <string>

namespace pas {
namespace visitor {

#define DESCEND(...)                                                           \
  {                                                                            \
    depth_ += 1;                                                               \
    __VA_ARGS__;                                                               \
    depth_ -= 1;                                                               \
  }

std::string Printer::get_indent() { return std::string(depth_, ' '); }

void Printer::visit(pas::ast::ProgramModule &pm) {
  stream_ << get_indent() << "ProgramModule name=" << pm.program_name_ << '\n';
  DESCEND(visit(pm.block_));
}

void Printer::visit(pas::ast::Block &block) {
  stream_ << get_indent() << "Block" << '\n';

  assert(block.decls_.get() != nullptr);
  DESCEND(visit(*block.decls_.get()));

  DESCEND(visit(block.stmt_seq_));
}

void Printer::visit(pas::ast::Declarations &decls) {
  stream_ << get_indent() << "Declarations" << '\n';

  for (auto &const_def : decls.const_defs_) {
    DESCEND(visit(const_def));
  }

  for (auto &type_def : decls.type_defs_) {
    DESCEND(visit(type_def));
  }

  for (auto &var_decl : decls.var_decls_) {
    DESCEND(visit(var_decl));
  }

  for (auto &subprog_decl : decls.subprog_decls_) {
    DESCEND(visit(subprog_decl));
  }
}

void Printer::visit(pas::ast::ConstDef &const_def) {
  stream_ << get_indent() << "ConstDef name=" << const_def.ident_ << '\n';

  DESCEND(visit(const_def.const_expr_));
}

void Printer::visit(pas::ast::TypeDef &type_def) {
  stream_ << get_indent() << "TypeDef name=" << type_def.ident_ << '\n';

  DESCEND(visit(type_def.type_));
}

void Printer::visit(pas::ast::VarDecl &var_decl) {
  stream_ << get_indent() << "VarDecl names=";
  bool first = true;
  for (auto &name : var_decl.ident_list_) {
    if (!first) {
      stream_ << ',';
    }
    stream_ << name;
    first = false;
  }
  stream_ << '\n';

  DESCEND(visit(var_decl.type_));
}

void Printer::visit(pas::ast::ConstExpr &const_expr) {
  stream_ << get_indent() << "ConstExpr";
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

  DESCEND(visit(const_expr.factor_));
}

void Printer::visit(pas::ast::ConstFactor &const_factor) {
  stream_ << get_indent() << "ConstFactor ";
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

// TODO: implement these properly.
void Printer::visit([[maybe_unused]] pas::ast::Type &node) {}
void Printer::visit([[maybe_unused]] pas::ast::ArrayType &node) {}
void Printer::visit([[maybe_unused]] pas::ast::Subrange &node) {}
void Printer::visit([[maybe_unused]] pas::ast::RecordType &node) {}
void Printer::visit([[maybe_unused]] pas::ast::SetType &node) {}
void Printer::visit([[maybe_unused]] pas::ast::PointerType &node) {}
void Printer::visit([[maybe_unused]] pas::ast::FieldList &node) {}

void Printer::visit(pas::ast::Assignment &assignment) {
  stream_ << get_indent() << "Assignment" << '\n';

  DESCEND(visit(assignment.designator_));
  DESCEND(visit(assignment.expr_));
}

void Printer::visit(pas::ast::ProcCall &proc_call) {
  stream_ << get_indent() << "ProcCall name=" << proc_call.proc_ident_ << '\n';

  for (pas::ast::Expr &expr : proc_call.params_) {
    DESCEND(visit(expr));
  }
}

void Printer::visit(pas::ast::StmtSeq &stmt_seq) {
  stream_ << get_indent() << "StmtSeq" << '\n';

  for (auto &stmt : stmt_seq.stmts_) {
    visit(stmt);
  }
}

void Printer::visit(pas::ast::IfStmt &if_stmt) {
  stream_ << get_indent() << "IfStmt" << '\n';

  DESCEND(visit(if_stmt.cond_expr_));
  DESCEND(visit(if_stmt.then_stmt_));
  if (if_stmt.else_stmt_.has_value()) {
    DESCEND(visit(if_stmt.else_stmt_.value()));
  }
}

// TODO: implement these properly.
void Printer::visit([[maybe_unused]] pas::ast::CaseStmt &node) {}
void Printer::visit([[maybe_unused]] pas::ast::Case &node) {}

void Printer::visit(pas::ast::WhileStmt &while_stmt) {
  stream_ << get_indent() << "WhileStmt" << '\n';

  DESCEND(visit(while_stmt.cond_expr_));
  DESCEND(visit(while_stmt.inner_stmt_));
}

void Printer::visit([[maybe_unused]] pas::ast::RepeatStmt &node) {}

void Printer::visit(pas::ast::ForStmt &for_stmt) {
  stream_ << get_indent() << "ForStmt ident=" << for_stmt.ident_ << " ";
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

  DESCEND(visit(for_stmt.start_val_expr_));
  DESCEND(visit(for_stmt.finish_val_expr_));
  DESCEND(visit(for_stmt.inner_stmt_));
}

void Printer::visit(pas::ast::Designator &designator) {
  stream_ << get_indent() << "Designator ident=" << designator.ident_ << '\n';

  for (pas::ast::DesignatorItem &item : designator.items_) {
    DESCEND(visit(item));
  }
}

void Printer::visit(pas::ast::DesignatorItem &designator_item) {
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
void Printer::visit(pas::ast::DesignatorArrayAccess &array_access) {
  stream_ << get_indent() << "DesignatorArrayAccess" << '\n';

  for (std::unique_ptr<pas::ast::Expr> &expr_ptr : array_access.expr_list_) {
    DESCEND(visit(*expr_ptr));
  }
}

// TODO: implement these properly.
void Printer::visit(
    [[maybe_unused]] pas::ast::DesignatorFieldAccess &field_access) {}
void Printer::visit(
    [[maybe_unused]] pas::ast::DesignatorPointerAccess &pointer_access) {}
void Printer::visit([[maybe_unused]] pas::ast::MemoryStmt &node) {}
void Printer::visit([[maybe_unused]] pas::ast::EmptyStmt &empty_stmt) {}

void Printer::visit(pas::ast::Stmt &stmt) {
  std::visit(
      [this](auto &stmt_alt) {
        // stmt_alt is unique_ptr, so let's dereference it.
        visit(*stmt_alt.get()); // Printer::visit(stmt_alt);
      },
      stmt);
}

void Printer::visit(pas::ast::Expr &expr) {
  if (!expr.op_.has_value()) {
    visit(expr.start_expr_);
  } else {
    stream_ << get_indent() << "Expr op=";
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

    DESCEND(visit(expr.start_expr_));
    DESCEND(visit(expr.op_.value().expr));
  }
}

void Printer::visit(pas::ast::SimpleExpr &simple_expr) {
  if (simple_expr.ops_.empty() && !simple_expr.unary_op_.has_value()) {
    visit(simple_expr.start_term_);
  } else {
    stream_ << get_indent() << "SimpleExpr";
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

    DESCEND(visit(simple_expr.start_term_));

    for (pas::ast::SimpleExpr::Op &op : simple_expr.ops_) {
      switch (op.op) {
      case pas::ast::AddOp::Minus: {
        DESCEND(stream_ << get_indent() << "Minus" << '\n');
        break;
      }
      case pas::ast::AddOp::Plus: {
        DESCEND(stream_ << get_indent() << "Plus" << '\n');
        break;
      }
      case pas::ast::AddOp::Or: {
        DESCEND(stream_ << get_indent() << "Or" << '\n');
        break;
      }
      default:
        assert(false);
        __builtin_unreachable();
      }

      DESCEND(visit(op.term));
    }
  }
}

void Printer::visit(pas::ast::Term &term) {
  if (term.ops_.empty()) {
    visit(term.start_factor_);
  } else {
    stream_ << get_indent() << "Term" << '\n';

    DESCEND(visit(term.start_factor_));

    for (pas::ast::Term::Op &op : term.ops_) {
      switch (op.op) {
      case pas::ast::MultOp::And: {
        DESCEND(stream_ << get_indent() << "And" << '\n');
        break;
      }
      case pas::ast::MultOp::IntDiv: {
        DESCEND(stream_ << get_indent() << "IntDiv" << '\n');
        break;
      }
      case pas::ast::MultOp::Modulo: {
        DESCEND(stream_ << get_indent() << "Modulo" << '\n');
        break;
      }
      case pas::ast::MultOp::Multiply: {
        DESCEND(stream_ << get_indent() << "Multiply" << '\n');
        break;
      }
      case pas::ast::MultOp::RealDiv: {
        DESCEND(stream_ << get_indent() << "RealDiv" << '\n');
        break;
      }
      default:
        assert(false);
        __builtin_unreachable();
      }

      DESCEND(visit(op.factor));
    }
  }
}
void Printer::visit(pas::ast::Factor &factor) {
  switch (factor.index()) {
  case get_idx(pas::ast::FactorKind::Bool): {
    bool value = std::get<bool>(factor);
    stream_ << get_indent() << "Factor Bool" << (value ? "true" : "false")
            << '\n';
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
    stream_ << get_indent() << "Factor Not" << '\n';

    DESCEND(visit(std::get<pas::ast::NegationUP>(factor)->factor_));
    break;
  }
  case get_idx(pas::ast::FactorKind::Nil): {
    stream_ << get_indent() << "Factor Nil" << '\n';
    break;
  }
  case get_idx(pas::ast::FactorKind::Number): {
    stream_ << get_indent() << "Factor number=" << std::get<int>(factor)
            << '\n';
    break;
  }
  case get_idx(pas::ast::FactorKind::String): {
    stream_ << get_indent() << "Factor string=\""
            << std::get<std::string>(factor) << "\"" << '\n';
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

void Printer::visit(pas::ast::FuncCall &func_call) {
  stream_ << get_indent() << "FuncCall ident=" << func_call.func_ident_ << '\n';
  for (pas::ast::Expr &expr : func_call.params_) {
    DESCEND(visit(expr));
  }
}

void Printer::visit([[maybe_unused]] pas::ast::Element &node) {}
void Printer::visit([[maybe_unused]] pas::ast::SubprogDecl &node) {}
void Printer::visit([[maybe_unused]] pas::ast::ProcDecl &node) {}
void Printer::visit([[maybe_unused]] pas::ast::FuncDecl &node) {}
void Printer::visit([[maybe_unused]] pas::ast::ProcHeading &node) {}
void Printer::visit([[maybe_unused]] pas::ast::FormalParam &node) {}

void Printer::visit(pas::ast::CompilationUnit &cu) { visit(cu.pm_); }

#undef DESCEND

} // namespace visitor
} // namespace pas
