#pragma once

#include <unordered_map>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

#include "ast/ast.hpp"
#include "ast/utils/get_idx.hpp"
#include "ast/visit.hpp"

namespace pas {
namespace visitor {

// Примеры IR-а.
//   https://mcyoung.xyz/2023/08/01/llvm-ir/

class Lowerer {
public:
  static void initialize_for_native_target() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
  }

  Lowerer(llvm::LLVMContext &context, const std::string &file_name,
          pas::ast::CompilationUnit &cu);

private:
  MAKE_VISIT_STMT_FRIEND();

  void visit(pas::ast::CompilationUnit &cu);
  void visit(pas::ast::ProgramModule &pm);
  void visit_toplevel(pas::ast::Block &block);

  void process_decls(pas::ast::Declarations &decls);
  void process_type_def(pas::ast::TypeDef &type_def);
  void process_var_decl(pas::ast::VarDecl &var_decl);

  struct PointerType;
  enum class TypeKind : size_t {
    // Base types
    Integer = 0,
    Char = 1,
    String = 2,

    // Pointer to an existing type
    //        PointerType = 2
  };

  // Always unveiled, synonims are expanded, when type is added to the
  // identifier mapping.
  using Type = std::variant<std::monostate, std::monostate,
                            std::monostate>; //, std::shared_ptr<PointerType>>;
                                             //    struct PointerType {
  //        // Types must be referenced as shared_ptrs:
  //        //   the objects in unordered_map (and plain map, almost any
  //        container)
  //        //   can move around memory, we have to account for that.
  //        // For an unordered map, if it is a hashtable, it happens, then
  //        //   there's too much chance of collision, we have to expand
  //        //   storage so that items are evenly spread.
  //        std::shared_ptr<Type> ref_type;
  //    };

  enum class ValueKind : size_t {
    // Base types
    Integer = 0,
    Char = 1,
    String = 2,

    //          // Pointer to another value.
    //          Pointer = 3
  };

private:
  void visit(pas::ast::MemoryStmt &memory_stmt);
  void visit(pas::ast::RepeatStmt &repeat_stmt);
  void visit(pas::ast::CaseStmt &case_stmt);

  void visit(pas::ast::StmtSeq &stmt_seq);

  void visit(pas::ast::IfStmt &if_stmt);

  void visit(pas::ast::EmptyStmt &empty_stmt);

  void visit(pas::ast::ForStmt &for_stmt);

  void visit(pas::ast::Assignment &assignment);
  void visit(pas::ast::ProcCall &proc_call);

  void visit(pas::ast::WhileStmt &while_stmt);

private:
  using IRRegister = std::string;
  using IRFuncName = std::string;
  using PascalIdent = std::string;

  template <typename T> struct EraseFromParent {
    void operator()(T *ptr) { ptr->eraseFromParent(); }
  };

  llvm::LLVMContext &context_;
  std::unique_ptr<llvm::Module> module_uptr_;

  llvm::Function *current_func_ = nullptr;

  // static EraseFromParent<llvm::Function> FunctionDeleter;
  // std::unique_ptr<llvm::Function, decltype(FunctionDeleter)> main_func_uptr_;

  // Храним по идентификатору паскаля регистр, в котором лежит значение.
  //   Это нужно для кодогенерации внутри функции.
  // Причем в IR, по аналогии с ассемблером, нет перекрытия (shadowing,
  //   как -Wshadow), т.к. перед нами не переменные, а регистры. Повторное
  //   указание каких либо действий с регистром влечет перезапись, а не
  //   создание нового регистра. Это и понятно, в IR как таковом нет
  //   областей видимости (scopes), кроме, возможно, функций.
  std::unordered_map<PascalIdent, std::pair<IRRegister, ValueKind>>
      cg_local_vars_;
  std::unordered_map<PascalIdent, std::pair<IRRegister, ValueKind>>
      cg_global_vars_;

  // Вызовы других функций обрабатываем так: название функции паскаля просто
  //   переделывается в ассемблер (mangling). Или даже вставляется как есть,
  //   или в паскале нет перегрузок.
  // В этой переменной хранятся функции, которые уже можно вызывать,
  //   при кодогенерации некоторой функции. Т.е. ее саму и всех, кто шел в
  //   файле до нее или был объявлен до нее.
  std::vector<std::unordered_map<PascalIdent, IRFuncName>> cg_ready_funcs_;

  // От прежнего интерпретатора, который вычислял значения, нам нужна только
  // проверка типов.
  //   Ведь весь рецепт вычисления записан в IR как в ассемблере. Причем рецепт
  //   один и тот же для разных значений тех же типов. Но проверка типов в
  //   широком смысле: нельзя прибавлять к типу, должна быть переменная или
  //   константа (immediate const, типо 5 и 2 в 5+2); не только то, что в
  //   выражениях типо 5+2 с обеих сторон числа.
  // Во время проверки типов рекурсивной производится и сама кодонерегация.
  std::vector<
      std::unordered_map<PascalIdent, std::variant<TypeKind, ValueKind>>>
      pascal_scopes_;

  // Чтобы посмотреть в действии, как работает трансляция, посмотрите видео
  // Андреаса Клинга.
  //   Он делал jit-компилятор javascript в браузере ladybird.
  //   https://www.youtube.com/watch?v=8mxubNQC5O8
};

} // namespace visitor
} // namespace pas
