LowererErrorOr<void> Lowerer::visit(pas::ast::StmtSeq &stmt_seq) {
  for (pas::ast::Stmt &stmt : stmt_seq.stmts_) {
    TRY(std::visit(
        [this](auto &stmt_alt) {
          // stmt_alt is unique_ptr, so let's dereference it.
          TRY(visit(*stmt_alt.get())); // Printer::visit(stmt_alt);
        },
        stmt));
  }
}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::MemoryStmt &memory_stmt) {}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::RepeatStmt &repeat_stmt) {}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::CaseStmt &case_stmt) {}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::IfStmt &if_stmt) {}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::EmptyStmt &empty_stmt) {}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::ForStmt &for_stmt) {}

LowererErrorOr<void>
Lowerer::visit([[maybe_unused]] pas::ast::WhileStmt &while_stmt) {}

LowererErrorOr<void> Lowerer::visit(pas::ast::Assignment &assignment) {
  llvm::Value *new_value = TRY(eval(assignment.expr_));
  pas::ast::Designator &designator = assignment.designator_;

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

  current_func_builder_->CreateStore(new_value, base_value);
}

LowererErrorOr<void> Lowerer::visit(pas::ast::ProcCall &proc_call) {
  const std::string &proc_name = proc_call.proc_ident_;

  // make constexpr vectors of builtins. Built-in procedures (name, visit
  // function) pairs.
  if (proc_name == "write_int") {
    TRY(visit_write_int(proc_call));
  } else if (proc_name == "write_str") {
    TRY(visit_write_str(proc_call));
  } else {
    throw pas::NotImplementedException(
        "procedure calls are not supported yet, except write_int");
  }
}
