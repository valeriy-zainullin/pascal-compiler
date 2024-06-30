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

  // Переобъявить встроенные получится. Но это на совести пользователя.

  // TODO: store all lowerer errors in std::vector of errors,
  //   catch them somewhere we can do restoration.
  // Although still generated code is not considered valid,
  //   if there are any errors.
  visit(cu);
}

LowererErrorOr<void> Lowerer::declare_builtins() {
  TRY(declare_builtin_types());

  TRY(declare_builtin_intio());
  TRY(declare_builtin_strio());
}

std::unique_ptr<llvm::Module> Lowerer::release_module() {
  return std::move(module_uptr_);
}

} // namespace visitor
} // namespace pas
