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
#include "ast/visit.hpp"

#include "exceptions.hpp"

namespace pas {
namespace visitor {

// auto Lowerer::FunctionDeleter = decltype(Lowerer::FunctionDeleter)();

Lowerer::Lowerer(llvm::LLVMContext &context, const std::string &file_name,
                 pas::ast::CompilationUnit &cu)
    : context_(context) {

  // ; ModuleID = 'top'
  // source_filename = "top"
  module_uptr_ = std::make_unique<llvm::Module>("top", context_);

  // Заводим пространство имен предопределенных символов.
  //   Это встроенные типы и встроенные функции.
  pascal_scopes_.emplace_back();

  // Add unique original names for basic types.
  pascal_scopes_.back()["Integer"] = TypeKind::Integer;
  pascal_scopes_.back()["Char"] = TypeKind::Char;
  pascal_scopes_.back()["String"] = TypeKind::String;

  // Заводим глобальное пространство имен, его контролирует программа.
  pascal_scopes_.emplace_back();

  // Вообще обработку различных типов данных можно
  //   вынести в отдельные файлы. Чтобы менять ir интерфейс
  //   (алгоритмы создания кода, кодирования операций) типа
  //   локально, а не бегая по этому большому файлу.

  visit(cu);
}

std::unique_ptr<llvm::Module> Lowerer::release_module() {
  return std::move(module_uptr_);
}

void Lowerer::visit(pas::ast::CompilationUnit &cu) { visit(cu.pm_); }

void Lowerer::visit(pas::ast::ProgramModule &pm) { visit_toplevel(pm.block_); }

void Lowerer::visit_toplevel(pas::ast::Block &block) {
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
    visit_stmt(*this, stmt);
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

void Lowerer::process_type_def(pas::ast::TypeDef &type_def) {}

llvm::Type *Lowerer::get_llvm_type_by_lang_type(TypeKind type) {
  switch (type) {
  case TypeKind::Integer:
    return current_func_builder_->getInt32Ty();
  case TypeKind::Char:
    return current_func_builder_->getInt8Ty();
  case TypeKind::String:
    return current_func_builder_->getInt8Ty()->getPointerTo();
    // case TypeKind::Pointer: return current_func_builder_->getPtrTy();

  default:
    assert(false);
    __builtin_unreachable();
  }
}

llvm::AllocaInst *Lowerer::codegen_alloc_value_of_type(TypeKind type) {
  return current_func_builder_->CreateAlloca(get_llvm_type_by_lang_type(type));
}

pas::visitor::Lowerer::TypeKind
Lowerer::make_type_from_ast_type(pas::ast::Type &ast_type) {
  size_t type_index = ast_type.index();

  switch (type_index) {
  case get_idx(pas::ast::TypeKind::Array):
  case get_idx(pas::ast::TypeKind::Record):
  case get_idx(pas::ast::TypeKind::Set): {
    //      throw NotImplementedException(
    //          "only pointer types, basic types (Integer, Char) and their
    //          synonims " "are supported for now");
    throw pas::NotImplementedException(
        "only basic types (Integer, Char) and strings are supported for now");
  }
  case get_idx(pas::ast::TypeKind::Pointer): {
    throw pas::NotImplementedException("pointer types are not supported now");
  }

  //    case get_idx(pas::ast::TypeKind::Pointer): {
  //      const auto &ptr_type_up = std::get<pas::ast::PointerTypeUP>(type);
  //      const pas::ast::PointerType &ptr_type = *ptr_type_up;
  //      if (!ident_to_item_.contains(ptr_type.ref_type_name_)) {
  //        throw SemanticProblemException(
  //            "pointer type references an undeclared identifier: " +
  //            ptr_type.ref_type_name_);
  //      }
  //      std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item =
  //          ident_to_item_[ptr_type.ref_type_name_];
  //      if (item.index() != 0) {
  //        throw SemanticProblemException(
  //            "pointer type must reference a type, not a value: " +
  //            ptr_type.ref_type_name_);
  //      }
  //      auto type_item = std::get<std::shared_ptr<Type>>(item);
  //      auto pointer_type =
  //      std::make_shared<PointerType>(PointerType{type_item}); auto
  //      new_type_item = std::make_shared<Type>(pointer_type); return
  //      new_type_item; break;
  //    }
  case get_idx(pas::ast::TypeKind::Named): {
    const auto &named_type_up = std::get<pas::ast::NamedTypeUP>(ast_type);
    const pas::ast::NamedType &named_type = *named_type_up;
    std::optional<std::variant<TypeKind, Variable>> refd_type;
    for (auto &scope : pascal_scopes_) {
      if (scope.contains(named_type.type_name_)) {
        refd_type = scope[named_type.type_name_];
      }
    }

    if (!refd_type.has_value()) {
      throw pas::SemanticProblemException(
          "named type references an undeclared identifier: " +
          named_type.type_name_);
    }

    if (refd_type.value().index() != 0) {
      throw pas::SemanticProblemException(
          "named type must reference a type, not a value: " +
          named_type.type_name_);
    }

    auto type = std::get<TypeKind>(refd_type.value());
    return type;
  }
  default: {
    assert(false);
    __builtin_unreachable();
  }
  }
}

void Lowerer::process_var_decl(pas::ast::VarDecl &var_decl) {
  TypeKind var_type = make_type_from_ast_type(var_decl.type_);
  for (const std::string &ident : var_decl.ident_list_) {
    if (pascal_scopes_.back().contains(ident)) {
      throw pas::SemanticProblemException("identifier is already in use: " +
                                          ident);
    }
    pascal_scopes_.back()[ident] =
        Variable(codegen_alloc_value_of_type(var_type), std::move(var_type));
  }
}

void Lowerer::visit(pas::ast::MemoryStmt &memory_stmt) {}

void Lowerer::visit(pas::ast::RepeatStmt &repeat_stmt) {}

void Lowerer::visit(pas::ast::CaseStmt &case_stmt) {}

void Lowerer::visit(pas::ast::IfStmt &if_stmt) {}

void Lowerer::visit(pas::ast::EmptyStmt &empty_stmt) {}

void Lowerer::visit(pas::ast::ForStmt &for_stmt) {}

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

std::variant<Lowerer::TypeKind, Lowerer::Variable> *
Lowerer::lookup_decl(const std::string &identifier) {
  for (auto it = pascal_scopes_.rbegin(); it != pascal_scopes_.rend(); ++it) {
    auto &scope = *it;
    auto item_it = scope.find(identifier);
    if (item_it != scope.end()) {
      return &item_it->second;
    }
  }
  return nullptr;
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
    throw NotImplementedException(
        "string constants are not implemented for now..");
    // return current_func_builder_->getInt32(std::get<int>(factor));
    // return Value(std::get<std::string>(factor));
  }
  case get_idx(pas::ast::FactorKind::Nil): {
    throw NotImplementedException("Nil is not supported yet");
  }
  case get_idx(pas::ast::FactorKind::FuncCall): {
    throw NotImplementedException(
        "function call evaluation is not supported for now");
    // return eval(*std::get<pas::ast::FuncCallUP>(factor));
    break;
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

  // Value arg = eval(proc_call.params_[0]);

  // if (arg.index() != get_idx(ValueKind::String)) {
  //   throw SemanticProblemException(
  //       "procedure write_str parameter must be of type String");
  // }

  // std::cout << std::get<std::string>(arg);
}

void Lowerer::visit(pas::ast::WhileStmt &while_stmt) {}

} // namespace visitor
} // namespace pas
