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

llvm::Module* Lowerer::get_module() {
  return module_uptr_.get();
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
    throw pas::NotImplementedException(
        "const defs are not implemented yet");
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
  auto entry =
      llvm::BasicBlock::Create(context_, "entrypoint", main_func);
  main_func_builder.SetInsertPoint(entry);

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
    throw pas::NotImplementedException(
        "const defs are not implemented yet");
  }
  for (auto &type_def : decls.type_defs_) {
    process_type_def(type_def);
  }
  for (auto &var_decl : decls.var_decls_) {
    process_var_decl(var_decl);
  }
}

void Lowerer::process_type_def(pas::ast::TypeDef &type_def) {}

llvm::Type* Lowerer::get_llvm_type_by_lang_type(TypeKind type) {
  switch (type) {
    case TypeKind::Integer: return current_func_builder_->getInt32Ty();
    case TypeKind::Char:    return current_func_builder_->getInt8Ty();
    case TypeKind::String:  return current_func_builder_->getInt8Ty()->getPointerTo();
    // case TypeKind::Pointer: return current_func_builder_->getPtrTy();

    default:
      assert(false);
      __builtin_unreachable();
    }
  }

llvm::AllocaInst* Lowerer::codegen_alloc_value_of_type(TypeKind type) {
  return current_func_builder_->CreateAlloca(get_llvm_type_by_lang_type(type));
}

pas::visitor::Lowerer::TypeKind Lowerer::make_type_from_ast_type(pas::ast::Type& ast_type) {
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
      for (auto& scope: pascal_scopes_) {
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
    pascal_scopes_.back()[ident] = Variable(codegen_alloc_value_of_type(var_type), std::move(var_type));
  }
}

void Lowerer::visit(pas::ast::MemoryStmt &memory_stmt) {}

void Lowerer::visit(pas::ast::RepeatStmt &repeat_stmt) {}

void Lowerer::visit(pas::ast::CaseStmt &case_stmt) {
}

void Lowerer::visit(pas::ast::IfStmt &if_stmt) {}

void Lowerer::visit(pas::ast::EmptyStmt &empty_stmt) {}

void Lowerer::visit(pas::ast::ForStmt &for_stmt) {}

void Lowerer::visit(pas::ast::Assignment &assignment) {}
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

void Lowerer::visit_write_int(pas::ast::ProcCall &proc_call) {
  if (proc_call.params_.size() != 1) {
    throw pas::SemanticProblemException(
        "procedure write_int accepts only one parameter of type Integer");
  }


  // llvm::Value* arg = eval(proc_call.params_[0]);

  // if (arg.index() != get_idx(TypeKind::Integer)) {
  //   throw pas::SemanticProblemException(
  //       "procedure write_int parameter must be of type Integer");
  // }
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
