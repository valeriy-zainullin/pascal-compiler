#pragma once

#include <const_expr.hpp>
#include <stmt.hpp>
#include <type.hpp>

#include <string>
#include <vector>

namespace pas {
namespace ast {

class VarDecl {
public:
  VarDecl() = default;
  VarDecl(VarDecl &&other) = default;
  VarDecl &operator=(VarDecl &&other) = default;

public:
  VarDecl(std::vector<std::string> ident_list, Type type)
      : ident_list_(std::move(ident_list)), type_(std::move(type)) {}

public:
  std::vector<std::string> ident_list_;
  Type type_;
};

class ConstDef {
public:
  ConstDef() = default;
  ConstDef(ConstDef &&other) = default;
  ConstDef &operator=(ConstDef &&other) = default;

public:
  ConstDef(std::string ident, ConstExpr const_expr)
      : ident_(std::move(ident)), const_expr_(std::move(const_expr)) {}

public:
  std::string ident_;
  ConstExpr const_expr_;
};

class TypeDef {
public:
  TypeDef() = default;
  TypeDef(TypeDef &&other) = default;
  TypeDef &operator=(TypeDef &&other) = default;

public:
  TypeDef(std::string ident, Type type)
      : ident_(std::move(ident)), type_(std::move(type)) {}

public:
  std::string ident_;
  Type type_;
};

class FormalParam {
public:
  FormalParam() = default;
  FormalParam(FormalParam &&other) = default;
  FormalParam &operator=(FormalParam &&other) = default;

public:
  FormalParam(std::vector<std::string> proc_name, std::string type_ident)
      : proc_name_(std::move(proc_name)), type_ident_(std::move(type_ident)) {}

public:
  std::vector<std::string> proc_name_;
  std::string type_ident_;
};

class ProcHeading {
public:
  ProcHeading() = default;
  ProcHeading(ProcHeading &&other) = default;
  ProcHeading &operator=(ProcHeading &&other) = default;

public:
  ProcHeading(std::string proc_name, std::vector<FormalParam> params)
      : proc_name_(std::move(proc_name)), params_(std::move(params)) {}

public:
  std::string proc_name_;
  std::vector<FormalParam> params_;
};

class Declarations;
class Block {
public:
  Block() = default;
  Block(Block &&other) = default;
  Block &operator=(Block &&other) = default;

public:
  // If we move construct from these,
  //   there will be move-constructed arguments,
  //   then we'll move them to our fields.
  // If we copy construct, these are just copies,
  //   it's fine to take them over.
  Block(std::unique_ptr<Declarations> decls, std::vector<Stmt> stmt_seq)
      : decls_(std::move(decls)), stmt_seq_(std::move(stmt_seq)) {}

public:
  // Block is needed for FuncDecl and ProcDecl, but it needs Declarations,
  //   which in turn requires FuncDecl and ProcDecl as subprog decls.
  //   We can't store this object in each other as hierarchy requires as there's
  //   a cycle. Have to store a pointer. We can use unique_ptr.
  std::unique_ptr<Declarations> decls_;
  std::vector<Stmt> stmt_seq_;
};

class ProcDecl {
public:
  ProcDecl() = default;
  ProcDecl(ProcDecl &&other) = default;
  ProcDecl &operator=(ProcDecl &&other) = default;

public:
  ProcDecl(ProcHeading proc_heading, Block block)
      : proc_heading_(std::move(proc_heading)), block_(std::move(block)) {}

public:
  ProcHeading proc_heading_;
  Block block_;
};

class FuncDecl {
public:
  FuncDecl() = default;
  FuncDecl(FuncDecl &&other) = default;
  FuncDecl &operator=(FuncDecl &&other) = default;

public:
  FuncDecl(ProcDecl proc_decl, std::string ret_type_ident)
      : proc_decl_(std::move(proc_decl)),
        ret_type_ident_(std::move(ret_type_ident)) {}

public:
  ProcDecl proc_decl_;
  std::string ret_type_ident_;
};

enum class SubprogKind { Proc = 0, Func = 1 };

using SubprogDecl = std::variant<ProcDecl, FuncDecl>;

class Declarations {
public:
  Declarations() = default;
  Declarations(Declarations &&other) = default;
  Declarations &operator=(Declarations &&other) = default;

public:
  // If we move construct from these,
  //   there will be move-constructed arguments,
  //   then we'll move them to our fields.
  // If we copy construct, these are just copies,
  //   it's fine to take them over.
  Declarations(std::vector<ConstDef> const_defs, std::vector<TypeDef> type_defs,
               std::vector<VarDecl> var_decls,
               std::vector<SubprogDecl> subprog_decls)
      : const_defs_(std::move(const_defs)), type_defs_(std::move(type_defs)),
        var_decls_(std::move(var_decls)),
        subprog_decls_(std::move(subprog_decls)) {}

public:
  std::vector<ConstDef> const_defs_;
  std::vector<TypeDef> type_defs_;
  std::vector<VarDecl> var_decls_;
  std::vector<SubprogDecl> subprog_decls_;
};

} // namespace ast
} // namespace pas
