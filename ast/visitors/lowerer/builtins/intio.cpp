// TODO: also return type! And always return llvm::Value along with it's
//   pascal type.
// struct pas::Lowerer::Value {
//    pas::Type type;          // Fully expanded type. No typedefs inside, no
//    "named types". Goes to it's declaration, it's a property of this thing on
//    it's own, it will be reused. llvm::Value* evaluation; // Some instructions
//    or immediate constants.
// }
// Check it has specified type: value.type == pas::Type::BasicTypes::Integer
//   or value.type == pas::type::PointerType(BasicTypes::Char) for char*
llvm::Value *Lowerer::eval_read_int(pas::ast::FuncCall &func_call) {
  if (!func_call.params_.empty()) {
    throw SemanticProblemException(
        "function read_int doesn't accept parameters");
  }

  llvm::Function *read_int_func = module_uptr_->getFunction("read_int");
  // TODO: make this to be an assert.
  if (read_int_func == nullptr) {
    throw pas::RuntimeProblemException("compiler internal error: read_int was "
                                       "not declared, but it must have been");
  }
  return current_func_builder_->CreateCall(read_int_func,
                                           std::vector<llvm::Value *>());
}

void Lowerer::visit_write_int(pas::ast::ProcCall &proc_call) {
  if (proc_call.params_.size() != 1) {
    throw pas::SemanticProblemException(
        "procedure write_int accepts only one parameter of type Integer");
  }
  // TODO: typecheck the type.

  llvm::Value *arg = eval(proc_call.params_[0]);

  // if (arg.index() != get_idx(TypeKind::Integer)) {
  //   throw pas::SemanticProblemException(
  //       "procedure write_int parameter must be of type Integer");
  // }

  // https://stackoverflow.com/a/22310371
  llvm::Function *write_int_func = module_uptr_->getFunction("write_int");
  // TODO: make this to be an assert.
  if (write_int_func == nullptr) {
    throw pas::RuntimeProblemException("compiler internal error: write_int was "
                                       "not declared, but it must have been");
  }
  std::vector<llvm::Value *> write_int_args = {arg};
  current_func_builder_->CreateCall(write_int_func, write_int_args);
}
