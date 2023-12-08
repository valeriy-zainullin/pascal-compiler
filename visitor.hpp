#pragma once

#include <ast.hpp>
#include <visit.hpp>
#include <get_idx.hpp>

#include <unordered_map>

namespace pas {
namespace visitor {

class Printer {
public:
    Printer(std::ostream& stream)
        : stream_(stream) {}
private:
    void print_indent() {
        for (size_t i = 0; i < depth_; ++i) {
            stream_ << ' ';
        }
    }

    void visit(pas::ast::ProgramModule &pm) {
        print_indent();
        stream_ << "ProgramModule name=" << pm.program_name_ << '\n';
        depth_ += 1;
        visit(pm.block_);
        depth_ -= 1;
    }
    void visit(pas::ast::Block &block) {
        print_indent();
        stream_ << "Block" << '\n';
        depth_ += 1;
        assert(block.decls_.get() != nullptr);
        visit(*block.decls_.get());
        for (auto& stmt: block.stmt_seq_) {
            visit_stmt(*this, stmt);
        }
        depth_ -= 1;
    }
    void visit(pas::ast::Declarations &decls) {
        print_indent();
        stream_ << "Declarations" << '\n';
        depth_ += 1;
        for (auto& const_def: decls.const_defs_) {
            visit(const_def);
        }
        for (auto& type_def: decls.type_defs_) {
            visit(type_def);
        }
        for (auto& var_decl: decls.var_decls_) {
            visit(var_decl);
        }
        for (auto& subprog_decl: decls.subprog_decls_) {
            visit(subprog_decl);
        }
        depth_ -= 1;
    }
    void visit(pas::ast::ConstDef &const_def) {
        print_indent();
        stream_ << "ConstDef name=" << const_def.ident_ << '\n';
        depth_ += 1;
        visit(const_def.const_expr_);
        depth_ -= 1;
    }
    void visit(pas::ast::TypeDef &type_def) {
        print_indent();
        stream_ << "TypeDef name=" << type_def.ident_ << '\n';
        depth_ += 1;
        visit(type_def.type_);
        depth_ -= 1;
    }
    void visit(pas::ast::VarDecl &var_decl) {
        print_indent();
        stream_ << "VarDecl names=";
        bool first = true;
        for (auto& name: var_decl.ident_list_) {
            if (!first) {
                stream_ << ',';
            }
            stream_ << name;
            first = false;
        }
        stream_ << '\n';
        depth_ += 1;
        visit(var_decl.type_);
        depth_ -= 1;
    }
    void visit(pas::ast::ConstExpr &const_expr) {
        print_indent();
        stream_ << "ConstExpr" << '\n';
        depth_ += 1;
        if (const_expr.unary_op_.has_value()) {
            visit(const_expr.unary_op_.value());
        }
        visit(const_expr.factor_);
        depth_ -= 1;
    }
    void visit(pas::ast::ConstFactor &const_factor) {
        print_indent();
        stream_ << "ConstFactor ";
        switch (const_factor.index()) {
        case get_idx(pas::ast::ConstFactorKind::Identifier): {
            stream_ << "identifier " << const_factor.get<std::string>() << '\n';
            break;
        }
        case get_idx(pas::ast::ConstFactorKind::Number): {
            stream_ << "number " << const_factor.get<int>() << '\n';
            break;
        }
        case get_idx(pas::ast::ConstFactorKind::Bool): {
            stream_ << "bool " << (const_factor.get<bool>() ? "true" : "false") << '\n';
            break;
        }
        case get_idx(pas::ast::ConstFactorKind::Nil): {
            stream_ << "nil" << '\n';
            break;
        }
        default: assert(false); __builtin_unreachable();
        }
    }
    void visit(pas::ast::Type &node) {

    }
  void visit(pas::ast::ArrayType &node);
  void visit(pas::ast::Subrange &node);
  void visit(pas::ast::RecordType &node);
  void visit(pas::ast::SetType &node);
  void visit(pas::ast::PointerType &node);
  void visit(pas::ast::FieldList &node);
  void visit(pas::ast::Stmt &node);
  void visit(pas::ast::Assignment &node);
  void visit(pas::ast::ProcCall &node);
  void visit(pas::ast::IfStmt &node);
  void visit(pas::ast::CaseStmt &node);
  void visit(pas::ast::Case &node);
  void visit(pas::ast::WhileStmt &node);
  void visit(pas::ast::RepeatStmt &node);
  void visit(pas::ast::ForStmt &node);
  void visit(pas::ast::WhichWay &node);
  void visit(pas::ast::Designator &node);
  void visit(pas::ast::DesignatorItem &node);
  void visit(pas::ast::MemoryStmt &node);
  void visit(pas::ast::Expr &node);
  void visit(pas::ast::SimpleExpr &node);
  void visit(pas::ast::Term &node);
  void visit(pas::ast::Factor &node);
  void visit(pas::ast::FuncCall &node);
  void visit(pas::ast::Element &node);
  void visit(pas::ast::SubprogDecl &node);
  void visit(pas::ast::ProcDecl &node);
  void visit(pas::ast::FuncDecl &node);
  void visit(pas::ast::ProcHeading &node);
  void visit(pas::ast::FormalParam &node);
  void visit(pas::ast::UnaryOp &node);
  void visit(pas::ast::MultOp &node);
  void visit(pas::ast::AddOp &node);
  void visit(pas::ast::RelOp &node);
public:
  void visit(pas::ast::CompilationUnit &node);
private:
  std::ostream& stream_;
  size_t depth_ = 0;
};

class DescribedException : std::exception {
public:
    DescribedException() = delete;

    template <typename T>
    DescribedException(T msg)
        : msg_(std::move(msg)) {}

    virtual const char * what() const noexcept override {
        return msg_.c_str();
    }
private:
    std::string msg_;
};

class NotImplementedException : DescribedException {
public:
    using DescribedException::DescribedException;
};

class SemanticProblemException : DescribedException {
public:
    using DescribedException::DescribedException;
};

class Interpreter {
public:
    Interpreter() {
        // Add unique original names for basic types.
        ident_to_item_["Integer"] = std::make_shared<Type>(std::in_place_index<get_idx(TypeKind::Integer)>);
        ident_to_item_["Char"] = std::make_shared<Type>(std::in_place_index<get_idx(TypeKind::Char)>);
    }

    void interpret(pas::ast::CompilationUnit& cu) {
        interpret(cu.pm_);
    }

 private:
    void interpret(pas::ast::ProgramModule& pm) {
        interpret(pm.block_);
    }

    void interpret(pas::ast::Block& block) {
        // Decl field should always be there, it can just have
        //   no actual decls inside.
        assert(block.decls_.get() != nullptr);
        process_decls(*block.decls_);
        for (pas::ast::Stmt& stmt: block.stmt_seq_) {
            visit_stmt(*this, stmt);
        }
    }


    void process_decls(pas::ast::Declarations& decls) {
        if (!decls.subprog_decls_.empty()) {
            throw NotImplementedException("function decls are not implemented yet");
        }
        if (!decls.const_defs_.empty()) {
            throw NotImplementedException("const defs are not implemented yet");
        }
        for (auto& type_def: decls.type_defs_) {
            process_type_def(type_def);
        }
        for (auto& var_decl: decls.var_decls_) {
            process_var_decl(var_decl);
        }
    }

    struct PointerType;
    enum class TypeKind : size_t {
        // Base types
        Integer = 0,
        Char = 1,

        // Pointer to an existing type
        PointerType = 2
    };

    // Always unveiled, synonims are expanded, when type is added to the identifier mapping.
    using Type = std::variant<std::monostate, std::monostate, std::shared_ptr<PointerType>>;
    struct PointerType {
        // Types must be referenced as shared_ptrs:
        //   the objects in unordered_map (and plain map, almost any container)
        //   can move around memory, we have to account for that.
        // For an unordered map, if it is a hashtable, it happens, then
        //   there's too much chance of collision, we have to expand
        //   storage so that items are evenly spread.
        std::shared_ptr<Type> ref_type;
    };

    struct ValuePointer;
    enum class ValueKind : size_t {
        // Base types
        Integer = 0,
        Char = 1,

        // Pointer to another value.
        Pointer = 2
    };
    using Value = std::variant<int, char, std::unique_ptr<ValuePointer>>;
    struct ValuePointer {
        std::shared_ptr<Value> value;
    };

private:

    void process_type_def(const pas::ast::TypeDef& type_def) {
        size_t type_index = type_def.type_.index();
        switch (type_index) {
        case get_idx(pas::ast::TypeKind::Array):
        case get_idx(pas::ast::TypeKind::Record):
        case get_idx(pas::ast::TypeKind::Set): {
            throw NotImplementedException("only pointer types, basic types (Integer, Char) and their synonims are supported for now");
        }
        case get_idx(pas::ast::TypeKind::Pointer): {
            if (ident_to_item_.contains(type_def.ident_)) {
                throw SemanticProblemException("identifier is already in use: " + type_def.ident_);
            }
            const auto& ptr_type_up = std::get<pas::ast::PointerTypeUP>(type_def.type_);
            const pas::ast::PointerType& ptr_type = *ptr_type_up;
            if (!ident_to_item_.contains(ptr_type.ref_type_name_)) {
                throw SemanticProblemException("pointer type references an undeclared identifier: " + ptr_type.ref_type_name_);
            }
            std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item = ident_to_item_[ptr_type.ref_type_name_];
            if (item.index() != 0) {
                throw SemanticProblemException("pointer type must reference a type, not a value: " + ptr_type.ref_type_name_);
            }
            auto type_item = std::get<std::shared_ptr<Type>>(item);
            auto pointer_type = std::make_shared<PointerType>(PointerType{type_item});
            auto new_type_item = std::make_shared<Type>(pointer_type);
            auto new_item = std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>>(std::move(new_type_item));
            ident_to_item_[type_def.ident_] = new_item;
            break;
        }
        case get_idx(pas::ast::TypeKind::Named): {
            if (ident_to_item_.contains(type_def.ident_)) {
                throw SemanticProblemException("identifier is already in use: " + type_def.ident_);
            }
            const auto& named_type_up = std::get<pas::ast::NamedTypeUP>(type_def.type_);
            const pas::ast::NamedType& named_type = *named_type_up;
            if (!ident_to_item_.contains(named_type.type_name_)) {
                throw SemanticProblemException("named type references an undeclared identifier: " + named_type.type_name_);
            }
            std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item = ident_to_item_[named_type.type_name_];
            if (item.index() != 0) {
                throw SemanticProblemException("named type must reference a type, not a value: " + named_type.type_name_);
            }
            auto type_item = std::get<std::shared_ptr<Type>>(item);
            auto pointer_type = std::make_shared<PointerType>(PointerType{type_item});
            auto new_type_item = std::make_shared<Type>(pointer_type);
            auto new_item = std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>>(std::move(new_type_item));
            ident_to_item_[type_def.ident_] = new_item;
            break;
        }
        default: assert(false); __builtin_unreachable();
           }
    }

    // Won't pick std::shared_ptr<Type> up, because as a declaration
    //   this function is before Type declaration (using in private section).
    //   Inside of this function, all declarations are already processed
    //   and the body is already a definition.
    std::shared_ptr<Type> make_var_decl_type(const pas::ast::Type& type) {
        size_t type_index = type.index();
        switch (type_index) {
        case get_idx(pas::ast::TypeKind::Array):
        case get_idx(pas::ast::TypeKind::Record):
        case get_idx(pas::ast::TypeKind::Set): {
            throw NotImplementedException("only pointer types, basic types (Integer, Char) and their synonims are supported for now");
        }
        case get_idx(pas::ast::TypeKind::Pointer): {
            const auto& ptr_type_up = std::get<pas::ast::PointerTypeUP>(type);
            const pas::ast::PointerType& ptr_type = *ptr_type_up;
            if (!ident_to_item_.contains(ptr_type.ref_type_name_)) {
                throw SemanticProblemException("pointer type references an undeclared identifier: " + ptr_type.ref_type_name_);
            }
            std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item = ident_to_item_[ptr_type.ref_type_name_];
            if (item.index() != 0) {
                throw SemanticProblemException("pointer type must reference a type, not a value: " + ptr_type.ref_type_name_);
            }
            auto type_item = std::get<std::shared_ptr<Type>>(item);
            auto pointer_type = std::make_shared<PointerType>(PointerType{type_item});
            auto new_type_item = std::make_shared<Type>(pointer_type);
            return new_type_item;
            break;
        }
        case get_idx(pas::ast::TypeKind::Named): {
            const auto& named_type_up = std::get<pas::ast::NamedTypeUP>(type);
            const pas::ast::NamedType& named_type = *named_type_up;
            if (!ident_to_item_.contains(named_type.type_name_)) {
                throw SemanticProblemException("named type references an undeclared identifier: " + named_type.type_name_);
            }
            std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>> item = ident_to_item_[named_type.type_name_];
            if (item.index() != 0) {
                throw SemanticProblemException("named type must reference a type, not a value: " + named_type.type_name_);
            }
            auto type_item = std::get<std::shared_ptr<Type>>(item);
            auto pointer_type = std::make_shared<PointerType>(PointerType{type_item});
            auto new_type_item = std::make_shared<Type>(pointer_type);
            return new_type_item;
            break;
        }
        default: assert(false); __builtin_unreachable();
           }
    }

    std::shared_ptr<Value> make_uninit_value_of_type(std::shared_ptr<Type> type) {
        switch (type->index()) {
        case get_idx(TypeKind::Integer): {
            return std::make_shared<Value>(std::in_place_index<get_idx(ValueKind::Integer)>, 0);
            break;
        }
        case get_idx(TypeKind::Char): {
            return std::make_shared<Value>(std::in_place_index<get_idx(ValueKind::Char)>, '0');
            break;
        }
        case get_idx(TypeKind::PointerType): {
            return std::make_shared<Value>(
                        std::in_place_index<get_idx(ValueKind::Pointer)>,
                        make_uninit_value_of_type(
                            std::get<std::shared_ptr<PointerType>>(*type)->ref_type
                            )
                        );
            break;
        }
        default: assert(false); __builtin_unreachable();
        }
    }

    void process_var_decl(const pas::ast::VarDecl& var_decl) {
        std::shared_ptr<Type> var_type = make_var_decl_type(var_decl.type_);
        for (const std::string& ident: var_decl.ident_list_) {
            if (ident_to_item_.contains(ident)) {
                throw SemanticProblemException("identifier is already in use: " + ident);
            }
            std::shared_ptr<Value> value = make_uninit_value_of_type(var_type);
            ident_to_item_[ident] = std::make_shared<Value>(std::move(var_type), std::move(value));
        }
    }

    private:
    std::unordered_map<std::string, std::variant<std::shared_ptr<Type>, std::shared_ptr<Value>>> ident_to_item_;
};

} // namespace visitor
} // namespace pas
