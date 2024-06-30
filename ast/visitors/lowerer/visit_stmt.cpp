#include "ast/visitors/lowerer.hpp"

namespace pas {
namespace visitor {

LowererErrorOr<void> Lowerer::visit(const pas::ast::StmtSeq &stmt_seq) {
  for (const pas::ast::Stmt &stmt : stmt_seq.stmts_) {
    TRY(std::visit(
        [this](auto &stmt_alt) {
          // stmt_alt is unique_ptr, so let's dereference it.
          return visit(*stmt_alt.get()); // Lowerer::visit(stmt_alt);
        },
        stmt));
  }
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::MemoryStmt &memory_stmt) {
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::RepeatStmt &repeat_stmt) {
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::CaseStmt &case_stmt) {
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::IfStmt &if_stmt) {
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::EmptyStmt &empty_stmt) {
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::ForStmt &for_stmt) {
  return {};
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] const pas::ast::WhileStmt &while_stmt) {
  return {};
}

LowererErrorOr<void> Lowerer::visit(const pas::ast::Assignment &assignment) {
  llvm::Value *new_value = TRY(eval(assignment.expr_));
  // pas::ast::Designator &designator = assignment.designator_;

  // if (!ident_to_item_.contains(designator.ident_)) {
  //   throw SemanticProblemException(
  //       "assignment references an undeclared identifier: " +
  //       designator.ident_);
  // }

  // std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item =
  //     ident_to_item_[designator.ident_];
  // if (item.index() != 1) {
  //   throw SemanticProblemException(
  //       "assignment must reference a value, not a type: " +
  //       designator.ident_);
  // }
  // auto value = std::get<std::shared_ptr<Value>>(item);

  // if (!designator.items_.empty()) {

  //   if (value->index() != get_idx(ValueKind::String)) {
  //     throw SemanticProblemException(
  //         "pointer and array access are only allowed for strings");
  //   }

  //   if (designator.items_.size() >= 2) {
  //     throw SemanticProblemException(
  //         "a string may have only one array access in assignment");
  //   }

  //   if (designator.items_[0].index() !=
  //       get_idx(pas::ast::DesignatorItemKind::ArrayAccess)) {
  //     throw SemanticProblemException(
  //         "only direct and array accesses are supported in assignment");
  //   }

  //   auto &array_access =
  //       std::get<pas::ast::DesignatorArrayAccess>(designator.items_[0]);

  //   if (array_access.expr_list_.size() >= 2) {
  //     throw NotImplementedException(
  //         "array access for more than one index is not supported");
  //   }

  //   Value value_index = eval(*array_access.expr_list_[0]);
  //   if (value_index.index() != get_idx(ValueKind::Integer)) {
  //     throw SemanticProblemException(
  //         "can only do indexing with integer type");
  //   }

  //   if (new_value.index() != get_idx(ValueKind::Char)) {
  //     throw SemanticProblemException("string item assignment can only accept
  //     "
  //                                     "a Char on the right hand size");
  //   }

  //   std::get<std::string>(*value)[std::get<int>(value_index)] =
  //       std::get<char>(new_value);
  //   return;
  // }

  // if (new_value.index() != value->index()) {
  //   throw SemanticProblemException(
  //       "incompatible types, must be of the same type for assignment");
  // }

  // *value = new_value; // Copy assign a new value.

  // TODO: check if item with identifier exists in the first place!!
  // IMPORTANT!
  TRY(scopes_.check_ident_type(assignment.designator_.ident_,
                               pas::IdentType::Variable));
  Variable *var = scopes_.find_var(assignment.designator_.ident_);
  ASSERT(var != nullptr, "identifier type (is defined and is a variable) "
                         "checked above with check_ident_type");

  llvm::Value *base_value = var->memory;

  ir_builder_->CreateStore(new_value, base_value);

  return {};
}

LowererErrorOr<void> Lowerer::visit(const pas::ast::ProcCall &proc_call) {
  const std::string &proc_name = proc_call.proc_ident_;

  TRY(check_ident_type(proc_name, IdentType::Function));

  Function *func = scopes_.find_func(proc_name);

  // TODO: evaluate args, check types!!
  //   Also store argument names, so that it's possible to
  //   say what argument (by name) is wrong, not just argument
  //   index, expected type, call argument type.

  // TODO: somehow call eval(const pas::ast::FuncCall) here.
  //   To reuse the code. Just ignore the llvm::Value. It's
  //   created for each create call, but not necessarily stored
  //   as a named register, because it may be not needed.

  // TODO: write actual line number, instead of 000.
  ir_builder_->CreateCall(func->llvm_function, {}, "L000_" + proc_name);

  return {};
}

} // namespace visitor
} // namespace pas
