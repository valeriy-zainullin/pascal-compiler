#include "ast/visitors/lowerer.hpp"

#include <memory> // std::unique_ptr

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "ast/ast.hpp"
#include "ast/utils/get_idx.hpp"

#include "exceptions.hpp"

namespace pas {
namespace visitor {

// auto Lowerer::FunctionDeleter = decltype(Lowerer::FunctionDeleter)();

Lowerer::Lowerer(llvm::LLVMContext &context, const std::string &filepath,
                 pas::ast::CompilationUnit &cu)
    : context_(context) {

  // ; ModuleID = filepath
  // source_filename = filepath
  module_uptr_ = std::make_unique<llvm::Module>(filepath, context_);

  // Заводим пространство имен предопределенных символов.
  //   Это встроенные типы и встроенные функции.
  scopes_.push_scope();

  // Add unique original names for basic types.
  scopes_.store_typedef({"Integer", BasicType::Integer});
  scopes_.store_typedef({"Char", BasicType::Char});
  scopes_.store_typedef({"String", BasicType::String});

  // Move builtin types and builtin functions.
  // declare_builtins();

  // Переобъявить встроенные получится. Но это на совести пользователя.

  visit(cu);
}

std::unique_ptr<llvm::Module> Lowerer::release_module() {
  return std::move(module_uptr_);
}

void Lowerer::visit(pas::ast::CompilationUnit &cu) { visit(cu.pm_); }

void Lowerer::visit(pas::ast::ProgramModule &pm) { visit_toplevel(pm.block_); }

void Lowerer::visit_toplevel(pas::ast::Block &block) {
  // Заводим глобальное пространство имен, туда пойдут все объявления ниже.
  scopes_.push_scope();

  // Decl field should always be there, it can just have
  //   no actual decls inside.
  assert(block.decls_.get() != nullptr);

  if (!block.decls_->subprog_decls_.empty()) {
    throw pas::NotImplementedException(
        "function decls are not implemented yet");
  }
  if (!block.decls_->const_defs_.empty()) {
    throw pas::NotImplementedException("const defs are not implemented yet");
  }

  // All subfunctions were generated, let's codegen the main function.

  llvm::IRBuilder<> main_func_builder(context_);

  // declare void @main()
  llvm::FunctionType *main_func_type =
      llvm::FunctionType::get(main_func_builder.getInt32Ty(), false);
  // Create links object to parent. So it's deleted along with the parent.
  //   Won't free it in any specific way. It's a good think to have create.
  //   We didn't allocate with new, so we don't free it with delete. It's
  //   not our responsibility.
  auto main_func =
      llvm::Function::Create(main_func_type, llvm::Function::ExternalLinkage,
                             "main", module_uptr_.get());
  // https://stackoverflow.com/a/10444311

  // entrypoint:
  auto entry = llvm::BasicBlock::Create(context_, "entrypoint", main_func);
  main_func_builder.SetInsertPoint(entry);

  // Declare builtin functions.
  //   https://stackoverflow.com/a/22310371

  std::vector<llvm::Type *> write_int_args = {main_func_builder.getInt32Ty()};
  llvm::FunctionType *write_int_type = llvm::FunctionType::get(
      main_func_builder.getVoidTy(), write_int_args, false);
  llvm::Function::Create(write_int_type, llvm::Function::ExternalLinkage,
                         "write_int", module_uptr_.get());

  std::vector<llvm::Type *> write_str_args = {main_func_builder.getPtrTy()};
  llvm::FunctionType *write_str_type = llvm::FunctionType::get(
      main_func_builder.getVoidTy(), write_str_args, false);
  llvm::Function::Create(write_str_type, llvm::Function::ExternalLinkage,
                         "write_str", module_uptr_.get());

  // llvm::FunctionType выдалется с помощью placement new в памяти внутри
  //   контекста. Потому освободится вместе с контекстом. А наличие вызова
  //   деструктора санитайзеры видимо не проверяют, т.к. это
  //   библиотека, она уже скомпилирована и проверки туда не вставить.

  current_func_ = main_func;
  current_func_builder_ = &main_func_builder;

  for (auto &type_def : block.decls_->type_defs_) {
    process_type_def(type_def);
  }
  for (auto &var_decl : block.decls_->var_decls_) {
    process_var_decl(var_decl);
  }

  visit(block.stmt_seq_);

  current_func_builder_->CreateRet(current_func_builder_->getInt32(0));

  current_func_ = nullptr;
  current_func_builder_ = nullptr;
}

void Lowerer::visit(pas::ast::StmtSeq &stmt_seq) {
  for (pas::ast::Stmt &stmt : stmt_seq.stmts_) {
    std::visit(
        [this](auto &stmt_alt) {
          // stmt_alt is unique_ptr, so let's dereference it.
          visit(*stmt_alt.get()); // Printer::visit(stmt_alt);
        },
        stmt);
  }
}

void Lowerer::process_decls(pas::ast::Declarations &decls) {
  if (!decls.subprog_decls_.empty()) {
    throw pas::SemanticProblemException(
        "function decls are not allowed inside other functions");
  }
  if (!decls.const_defs_.empty()) {
    throw pas::NotImplementedException("const defs are not implemented yet");
  }
  for (auto &type_def : decls.type_defs_) {
    process_type_def(type_def);
  }
  for (auto &var_decl : decls.var_decls_) {
    process_var_decl(var_decl);
  }
}

void Lowerer::process_type_def([[maybe_unused]] pas::ast::TypeDef &type_def) {}

// llvm::AllocaInst *Lowerer::codegen_alloc_value_of_type(TypeKind type) {
//   return
//   current_func_builder_->CreateAlloca(get_llvm_type_by_lang_type(type));
// }

LowererErrorOr<void> Lowerer::process_var_decl(pas::ast::VarDecl &var_decl) {
  pas::ComputedType type = TRY(scopes_.compute_ast_type(var_decl.type_));
  for (const std::string &ident : var_decl.ident_list_) {
    pascal_scopes_.back()[ident] =
        Variable(codegen_alloc_value_of_type(var_type), std::move(var_type));
  }
}

void Lowerer::visit([[maybe_unused]] pas::ast::MemoryStmt &memory_stmt) {}

void Lowerer::visit([[maybe_unused]] pas::ast::RepeatStmt &repeat_stmt) {}

void Lowerer::visit([[maybe_unused]] pas::ast::CaseStmt &case_stmt) {}

void Lowerer::visit([[maybe_unused]] pas::ast::IfStmt &if_stmt) {}

void Lowerer::visit([[maybe_unused]] pas::ast::EmptyStmt &empty_stmt) {}

void Lowerer::visit([[maybe_unused]] pas::ast::ForStmt &for_stmt) {}

void Lowerer::visit(pas::ast::Assignment &assignment) {
  llvm::Value *new_value = eval(assignment.expr_);
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
  std::variant<TypeKind, Variable> *decl = lookup_decl(designator.ident_);
  if (decl == nullptr) {
    throw SemanticProblemException("declaration not found: " +
                                   designator.ident_);
  }
  if (decl->index() != 1) {
    throw SemanticProblemException(
        "designator must reference a value, not a type: " + designator.ident_);
  }
  Variable &variable = std::get<Variable>(*decl);
  llvm::Value *base_value = variable.allocation;

  current_func_builder_->CreateStore(new_value, base_value);
}

void Lowerer::visit(pas::ast::ProcCall &proc_call) {
  const std::string &proc_name = proc_call.proc_ident_;

  if (proc_name == "write_int") {
    visit_write_int(proc_call);
  } else if (proc_name == "write_str") {
    visit_write_str(proc_call);
  } else {
    throw pas::NotImplementedException(
        "procedure calls are not supported yet, except write_int");
  }
}

// TODO: say where a type assertion is checked in typechecker.
//   In a fixed format manner. Invent an intuitive format for it.

llvm::Value *Lowerer::eval(pas::ast::Factor &factor) {
  switch (factor.index()) {
  case get_idx(pas::ast::FactorKind::Bool): {
    // No dedicated bool type for now for simplicity
    //   (I don't have much time, too many things to do).
    return current_func_builder_->getInt1(std::get<bool>(factor));
  }
  case get_idx(pas::ast::FactorKind::Number): {
    return current_func_builder_->getInt32(std::get<int>(factor));
  }
  case get_idx(pas::ast::FactorKind::String): {
    // TODO: change these to be std::string pointers, that are
    //   manually allocated and deallocated by calling create_str,
    //   dispose_str.
    return current_func_builder_->CreateGlobalStringPtr(
        std::get<std::string>(factor));
    // return Value(std::get<std::string>(factor));
  }
  case get_idx(pas::ast::FactorKind::Nil): {
    throw NotImplementedException("Nil is not supported yet");
  }
  case get_idx(pas::ast::FactorKind::FuncCall): {
    return eval(*std::get<pas::ast::FuncCallUP>(factor));
  }

  case get_idx(pas::ast::FactorKind::Negation): {
    llvm::Value *inner_value =
        eval(std::get<pas::ast::NegationUP>(factor)->factor_);
    // if (inner_value.index() != 0) {
    //   throw SemanticProblemException(
    //       "Negation is only applicable to integer type");
    // }
    return current_func_builder_->CreateNot(inner_value);
  }
  case get_idx(pas::ast::FactorKind::Expr): {
    llvm::Value *inner_value = eval(*std::get<pas::ast::ExprUP>(factor));
    return inner_value;
  }
  case get_idx(pas::ast::FactorKind::Designator): {
    auto &designator = std::get<pas::ast::Designator>(factor);
    // TODO: check if item with identifier exists in the first place!!
    // IMPORTANT!
    std::variant<TypeKind, Variable> *decl = lookup_decl(designator.ident_);
    if (decl == nullptr) {
      throw SemanticProblemException("declaration not found: " +
                                     designator.ident_);
    }
    if (decl->index() != 1) {
      throw SemanticProblemException(
          "designator must reference a value, not a type: " +
          designator.ident_);
    }
    Variable &variable = std::get<Variable>(*decl);
    llvm::Value *base_value = variable.allocation;

    for (pas::ast::DesignatorItem &item : designator.items_) {
      throw NotImplementedException(
          "designator element access is not supported for now!");
      //   switch (item.index()) {
      //   case get_idx(pas::ast::DesignatorItemKind::FieldAccess): {
      //     throw NotImplementedException("field access is not implemented");
      //   }
      //   case get_idx(pas::ast::DesignatorItemKind::PointerAccess): {
      //     throw NotImplementedException("pointer access is not implemented");
      //   }
      //   case get_idx(pas::ast::DesignatorItemKind::ArrayAccess): {
      //     //          if (base_value.index() != get_idx(ValueKind::Pointer))
      //     {
      //     //            throw NotImplementedException(
      //     //                "value must be a pointer for array access");
      //     //          }
      //     if (base_value.index() != get_idx(ValueKind::String)) {
      //       throw NotImplementedException(
      //           "value must be a string for array access");
      //     }
      //     pas::ast::DesignatorArrayAccess &array_access =
      //         std::get<pas::ast::DesignatorArrayAccess>(item);
      //     if (array_access.expr_list_.size() != 1) {
      //       throw NotImplementedException(
      //           "array access for more than one index is not supported");
      //     }
      //     Value value_index = eval(*array_access.expr_list_[0]);
      //     if (value_index.index() != get_idx(ValueKind::Integer)) {
      //       throw SemanticProblemException(
      //           "can only do indexing with integer type");
      //     }
      //     int index = std::get<int>(value_index);
      //     auto &value = std::get<std::string>(base_value);
      //     if (index < 0 || index >= value.size()) {
      //       // Won't be reported if it's a compiler, not an interpreter.
      //       //   Only a thing like valgrind or memory sanitizer.
      //       throw RuntimeProblemException("index is out of bounds: " +
      //                                     std::to_string(index));
      //     }
      //     base_value = Value(std::in_place_type<char>, value[index]);
      //     break;
      //   }
      //   }
    }
    return current_func_builder_->CreateLoad(
        get_llvm_type_by_lang_type(variable.type), base_value,
        designator.ident_);
  }
  default:
    assert(false);
    __builtin_unreachable();
  }
}

llvm::Value *Lowerer::eval(pas::ast::Term &term) {
  llvm::Value *value = eval(term.start_factor_);
  for (pas::ast::Term::Op &op : term.ops_) {
    // if (value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    llvm::Value *rhs_value = eval(op.factor);
    // if (rhs_value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    switch (op.op) {
    case pas::ast::MultOp::And: {
      value = current_func_builder_->CreateLogicalAnd(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::IntDiv: {
      value = current_func_builder_->CreateSDiv(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::Modulo: {
      value = current_func_builder_->CreateSRem(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::Multiply: {
      // nsw, nuw and etc.
      //   https://stackoverflow.com/a/61210926
      value = current_func_builder_->CreateMul(value, rhs_value);
      break;
    }
    case pas::ast::MultOp::RealDiv: {
      throw NotImplementedException("real numbers are not supported");
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  return value;
}

llvm::Value *Lowerer::eval(pas::ast::SimpleExpr &simple_expr) {
  // NOTE: unary op is ignored for now.
  llvm::Value *value = eval(simple_expr.start_term_);
  for (pas::ast::SimpleExpr::Op &op : simple_expr.ops_) {
    // if (value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    llvm::Value *rhs_value = eval(op.term);
    // if (rhs_value.index() != 0) {
    //   throw SemanticProblemException("can only do math with integer type");
    // }
    switch (op.op) {
    case pas::ast::AddOp::Plus: {
      value = current_func_builder_->CreateAdd(value, rhs_value);
      break;
    }
    case pas::ast::AddOp::Minus: {
      value = current_func_builder_->CreateSub(value, rhs_value);
      break;
    }
    case pas::ast::AddOp::Or: {
      value = current_func_builder_->CreateLogicalOr(value, rhs_value);
      break;
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  return value;
}

llvm::Value *Lowerer::eval(pas::ast::Expr &expr) {
  llvm::Value *value = eval(expr.start_expr_);
  if (expr.op_.has_value()) {
    pas::ast::Expr::Op &op = expr.op_.value();
    switch (op.rel) {
    case pas::ast::RelOp::Equal: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpEQ(value, rhs_value);
    }
    case pas::ast::RelOp::GreaterEqual: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSGE(value, rhs_value);
    }
    case pas::ast::RelOp::Greater: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSGT(value, rhs_value);
    }
    case pas::ast::RelOp::LessEqual: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSLE(value, rhs_value);
    }
    case pas::ast::RelOp::Less: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpSLT(value, rhs_value);
    }
    case pas::ast::RelOp::NotEqual: {
      llvm::Value *rhs_value = eval(op.expr);
      return current_func_builder_->CreateICmpNE(value, rhs_value);
    }
    case pas::ast::RelOp::In: {
      // Dispose "value" here.
      throw NotImplementedException("relation \"in\" is not supported");
    }
    default:
      assert(false);
      __builtin_unreachable();
    }
  }
  return value;
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

void Lowerer::visit(pas::ast::WhileStmt &while_stmt) {}

} // namespace visitor
} // namespace pas
