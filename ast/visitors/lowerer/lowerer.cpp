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

} // namespace visitor
} // namespace pas
