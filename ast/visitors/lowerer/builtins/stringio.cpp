// String io (write_str, read_str).
// Visit functions for builtin procedures.

void Lowerer::visit_write_str(pas::ast::ProcCall &proc_call) {
  if (proc_call.params_.size() != 1) {
    throw pas::SemanticProblemException(
        "procedure write_str accepts only one parameter of type String");
  }

  llvm::Value *arg = eval(proc_call.params_[0]);

  // if (arg.index() != get_idx(TypeKind::String)) {
  //   throw pas::SemanticProblemException(
  //       "procedure write_str parameter must be of type String");
  // }

  llvm::Function *write_str_func = module_uptr_->getFunction(
      "write_str"); // TODO: make this to be an assert.
  if (write_str_func == nullptr) {
    throw pas::RuntimeProblemException("compiler internal error: write_str was "
                                       "not declared, but it must have been");
  }
  std::vector<llvm::Value *> write_str_args = {arg};
  current_func_builder_->CreateCall(write_str_func, write_str_args);
}
