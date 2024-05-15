class Lowerer {
public:
  Lowerer(std::ostream& ir_stream)
    : ir_stream_(ir_stream) {

    // Вкидываем глобальное пространство имен.
    pascal_scopes_.emplace_back();

    // Add unique original names for basic types.
    pascal_scopes_.back()["Integer"] = TypeKind::Integer;
    pascal_scopes_.back()["Char"] = TypeKind::Char;
    pascal_scopes_.back()["String"] = TypeKind::String;

    // Вообще обработку различных типов данных можно
    //   вынести в отдельные файлы. Чтобы менять ir интерфейс
    //   (алгоритмы создания кода, кодирования операций) типа
    //   локально, а не бегая по этому большому файлу.
  }

  void codegen(pas::ast::CompilationUnit &cu) { codegen(cu.pm_); }

private:
  MAKE_VISIT_STMT_FRIEND();

  void codegen(pas::ast::ProgramModule &pm) { codegen(pm.block_); }

  void codegen(pas::ast::Block &block) {
    // Decl field should always be there, it can just have
    //   no actual decls inside.
    assert(block.decls_.get() != nullptr);
    process_decls(*block.decls_);
    // TODO: отслеживать вложенность функций, пока вложенные функции запрещены.
    //   И вообще вызова функций пока нет, потому это еще не проблема. Но как
    //   функции появятся, об этом надо будет задуматься.
    for (pas::ast::Stmt &stmt : block.stmt_seq_) {
      visit_stmt(*this, stmt);
    }
  }

  void process_decls(pas::ast::Declarations &decls) {
    if (!decls.subprog_decls_.empty()) {
      throw NotImplementedException("function decls are not implemented yet");
    }
    if (!decls.const_defs_.empty()) {
      throw NotImplementedException("const defs are not implemented yet");
    }
    for (auto &type_def : decls.type_defs_) {
      process_type_def(type_def);
    }
    for (auto &var_decl : decls.var_decls_) {
      process_var_decl(var_decl);
    }
  }

  // Не очень хорошая модель памяти для паскаля, со счетчиком ссылок и т.п.
  //   Она лучше подойдет для джавы какой-то. А в паскале низкоуровневый
  //   доступ, надо эмулировать память, поддерживать движение указателей
  //   по памяти и т.п. Паскаль лучше сразу компилировать, а не
  //   интерпретировать, чтобы не эмулировать оперативную память.
  // Зато наш код в каком-то виде напоминает код typechecker-а для паскаля.
  //   Так ведь можно было бы и реализовать typechecker, ведь все типы
  //   известны. Заодно проверить, что под идентификаторами скрываются
  //   нужные объекты, когда тип, когда значение.

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

  //    struct ValuePointer;
  enum class ValueKind : size_t {
    // Base types
    Integer = 0,
    Char = 1,
    String = 2,

    //          // Pointer to another value.
    //          Pointer = 3
  };
  using Value =
      std::pair<std::string, ValueKind>; // IR register + value kind (evaluated type)

private:
  void visit(pas::ast::MemoryStmt &memory_stmt) {}
  void visit(pas::ast::RepeatStmt &repeat_stmt) {}
  void visit(pas::ast::CaseStmt &case_stmt) {}

  void visit(pas::ast::StmtSeq &stmt_seq) {}

  void visit(pas::ast::IfStmt &if_stmt) {}

  void visit(pas::ast::EmptyStmt &empty_stmt) {}

  void visit(pas::ast::ForStmt &for_stmt) {}

private:
  using IRRegister = std::string;
  using IRFuncName = std::string;
  using PascalIdent = std::string;

  // Храним по идентификатору паскаля регистр, в котором лежит значение.
  //   Это нужно для кодогенерации внутри функции.
  // Причем в IR, по аналогии с ассемблером, нет перекрытия (shadowing,
  //   как -Wshadow), т.к. перед нами не переменные, а регистры. Повторное
  //   указание каких либо действий с регистром влечет перезапись, а не
  //   создание нового регистра. Это и понятно, в IR как таковом нет
  //   областей видимости (scopes), кроме, возможно, функций.
  std::unordered_map<PascalIdent, std::pair<IRRegister, ValueKind>> cg_local_vars_; 
  std::unordered_map<PascalIdent, std::pair<IRRegister, ValueKind>> cg_global_vars_; 

  // Вызовы других функций обрабатываем так: название функции паскаля просто
  //   переделывается в ассемблер (mangling). Или даже вставляется как есть,
  //   или в паскале нет перегрузок.
  // В этой переменной хранятся функции, которые уже можно вызывать,
  //   при кодогенерации некоторой функции. Т.е. ее саму и всех, кто шел в
  //   файле до нее или был объявлен до нее.
  std::vector<std::unordered_map<PascalIdent, IRFuncName>> cg_ready_funcs_; 

  // От прежнего интерпретатора, который вычислял значения, нам нужна только проверка типов.
  //   Ведь весь рецепт вычисления записан в IR как в ассемблере. Причем рецепт один и тот
  //   же для разных значений тех же типов. Но проверка типов в широком смысле: нельзя
  //   прибавлять к типу, должна быть переменная или константа (immediate const, типо 5
  //   и 2 в 5+2); не только то, что в выражениях типо 5+2 с обеих сторон числа.
  // Во время проверки типов рекурсивной производится и сама кодонерегация.
  std::vector<std::unordered_map<PascalIdent, std::variant<TypeKind, ValueKind>>> pascal_scopes_; 

  // Чтобы посмотреть в действии, как работает трансляция, посмотрите видео Андреаса Клинга.
  //   Он делал jit-компилятор javascript в браузере ladybird.
  //   https://www.youtube.com/watch?v=8mxubNQC5O8

  std::ostream& ir_stream_;
};
