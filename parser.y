%skeleton "lalr1.cc"
%require "3.5"

%defines
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include "ast.hpp"
    #include "get_idx.hpp"
    #include <string>
    #include <utility>

    /* Forward declaration of classes in order to disable cyclic dependencies */
    class Scanner;
    class Driver;
}


%define parse.trace
%define parse.error verbose

%code {
    #include "ast.hpp"
    #include "driver.hh"
    #include "location.hh"

    /* Redefine parser to use our function from scanner */
    static yy::parser::symbol_type yylex(Scanner &scanner) {
        return scanner.ScanToken();
    }

		/* iostream output function for std::pair */
    template <typename T, typename U>
    static std::ostream& print_token(std::ostream& stream, const std::pair<T, U>& pair) {
        return stream << '(' << pair.first << ',' << pair.second << ')';
    }
		/* iostream output function for any other type. */
    template <typename T>
    static std::ostream& print_token(std::ostream& stream, const T& token) {
        return stream << token;
    }
}

%lex-param { Scanner &scanner }

%parse-param { Scanner &scanner }
%parse-param { Driver &driver }

%locations

%nonassoc IFPREC
%nonassoc ELSE

%define api.token.prefix {TOK_}
// token name in variable
%token
    EOF       0      "end of file"
    ASSIGN    ":="
    MINUS     "-"
    PLUS      "+"
    STAR      "*"
    SLASH     "/"
    DIV       "div"
    MOD       "mod"
    EQ        "="
    NEQ       "<>"
    LT        "<"
    GT        ">"
    LEQ       "<="
    GEQ       ">="
    IN        "in"
    NOT       "not"
    OR        "or"
    AND       "and"
    LPAREN    "("
    RPAREN    ")"
    LBRACKET  "["
    RBRACKET  "]"
    DOT       "."
    COMMA     ","
    SEMICOLON ";"

    ARRAY     "array"
    BEGIN     "begin"
    CASE      "case"
    CONST     "const"
    DO        "do"
    DOWNTO    "downto"
    ELSE      "else"
    END       "end"
    FILE      "file"
    FOR       "for"
    FUNCTION  "function"
    IF        "if"
    NIL       "nil"
    OF        "of"
    PACKED    "packed"
    PROCEDURE "procedure"
    PROGRAM   "program"
    RECORD    "record"
    REPEAT    "repeat"
    SET       "set"
    THEN      "then"
    TO        "to"
    TYPE      "type"
    UNTIL     "until"
    VAR       "var"
    WHILE     "while"
    WITH      "with"

    FALSE     "False"
    TRUE      "True"

    NEW       "New"
    DISPOSE   "Dispose"
;

// Type names, variable names, function names, record names, etc.
//   There are some predefined identifiers that can't be used.
%token <std::string> identifier "identifier"

// String constant
%token <std::string>               string "string"
%token <int>                       number "number"
%token <std::pair<char, char>>     CharSubrange   // 'a..z', no multibyte for now (and wide chars).
%token <char>                      CharacterConst // 'a', no multibyte characters for now.

%nterm <pas::ast::CompilationUnit>              CompilationUnit
%nterm <pas::ast::ProgramModule>                ProgramModule
%nterm <std::vector<std::string>>               IdentList
%nterm <pas::ast::Block>                        Block
%nterm <pas::ast::Declarations>                 Declarations
%nterm <std::vector<pas::ast::ConstDef>>        ConstantDefBlockOpt
%nterm <std::vector<pas::ast::TypeDef>>         TypeDefBlockOpt
%nterm <std::vector<pas::ast::VarDecl>>         VariableDeclBlockOpt
%nterm <std::vector<pas::ast::SubprogDecl>>     SubprogDeclListOpt
%nterm <std::vector<pas::ast::ConstDef>>        ConstantDefBlock
%nterm <std::vector<pas::ast::ConstDef>>        ConstantDefList
%nterm <std::vector<pas::ast::TypeDef>>         TypeDefBlock
%nterm <std::vector<pas::ast::TypeDef>>         TypeDefList
%nterm <std::vector<pas::ast::VarDecl>>         VariableDeclBlock
%nterm <std::vector<pas::ast::VarDecl>>         VariableDeclList
%nterm <pas::ast::ConstDef>                     ConstantDef
%nterm <pas::ast::TypeDef>                      TypeDef
%nterm <pas::ast::VarDecl>                      VariableDecl
%nterm <pas::ast::ConstExpr>                    ConstExpression
%nterm <std::optional<pas::ast::UnaryOp>>       UnaryOperatorOpt
%nterm <pas::ast::ConstFactor>                  ConstFactor
%nterm <pas::ast::Type>                         Type
%nterm <pas::ast::ArrayType>                    ArrayType
%nterm <std::vector<pas::ast::Subrange>>        SubrangeList
%nterm <pas::ast::Subrange>                     Subrange
%nterm <pas::ast::RecordType>                   RecordType
%nterm <pas::ast::SetType>                      SetType
%nterm <pas::ast::PointerType>                  PointerType
%nterm <std::vector<pas::ast::FieldList>>       FieldListSequence
%nterm <pas::ast::FieldList>                    FieldList
%nterm <std::vector<pas::ast::Stmt>>            StatementSequence
%nterm <std::vector<pas::ast::Stmt>>            StatementList
%nterm <pas::ast::Stmt>                         Statement
%nterm <pas::ast::Assignment>                   Assignment
%nterm <pas::ast::ProcCall>                     ProcedureCall
%nterm <std::vector<pas::ast::Expr>>            ActualParametersOpt
%nterm <pas::ast::IfStmt>                       IfStatement
%nterm <pas::ast::CaseStmt>                     CaseStatement
%nterm <std::vector<pas::ast::Case>>            CaseList
%nterm <pas::ast::Case>                         Case
%nterm <std::vector<pas::ast::ConstExpr>>       CaseLabelList
%nterm <pas::ast::WhileStmt>                    WhileStatement
%nterm <pas::ast::RepeatStmt>                   RepeatStatement
%nterm <pas::ast::ForStmt>                      ForStatement
%nterm <pas::ast::WhichWay>                     WhichWay
%nterm <pas::ast::Designator>                   Designator
%nterm <std::vector<pas::ast::DesignatorItem>>  DesignatorStuffOpt
%nterm <std::vector<pas::ast::DesignatorItem>>  DesignatorStuff
%nterm <pas::ast::DesignatorItem>               DesignatorItem
%nterm <std::vector<pas::ast::Expr>>            ActualParameters
%nterm <std::vector<pas::ast::Expr>>            ExpList
%nterm <pas::ast::MemoryStmt>                   MemoryStatement
%nterm <pas::ast::Expr>                         Expression
%nterm <pas::ast::SimpleExpr>                   SimpleExpression
// TermList
%nterm <pas::ast::Term>                         Term
// FactorList
%nterm <pas::ast::Factor>                       Factor
%nterm <std::vector<pas::ast::Element>>         Setvalue
%nterm <std::vector<pas::ast::Element>>         ElementListOpt
%nterm <std::vector<pas::ast::Element>>         ElementList
%nterm <pas::ast::FuncCall>                     FunctionCall
%nterm <pas::ast::Element>                      Element
%nterm <std::vector<pas::ast::SubprogDecl>>     SubprogDeclList
%nterm <pas::ast::SubprogDecl>                  SubprogDecl
%nterm <pas::ast::ProcDecl>                     ProcedureDecl
%nterm <pas::ast::FuncDecl>                     FunctionDecl
%nterm <pas::ast::ProcHeading>                  ProcedureHeading
%nterm <std::vector<pas::ast::FormalParam>>     FormalParametersOpt
%nterm <pas::ast::ProcHeading>                  FunctionHeading
%nterm <std::vector<pas::ast::FormalParam>>     FormalParameters
%nterm <std::vector<pas::ast::FormalParam>>     OneFormalParamList
%nterm <pas::ast::FormalParam>                  OneFormalParam
%nterm <pas::ast::UnaryOp>                      UnaryOperator
%nterm <pas::ast::MultOp>                       MultOperator
%nterm <pas::ast::AddOp>                        AddOperator
%nterm <pas::ast::RelOp>                        Relation

%nterm <std::pair<std::vector<pas::ast::Term>,   std::vector<pas::ast::AddOp>>>  TermList
%nterm <std::pair<std::vector<pas::ast::Factor>, std::vector<pas::ast::MultOp>>> FactorList


// Prints output in parsing option for debugging location terminal
// %printer { print_token(yyo, $$); } <*>;

// Dangling else problem in pascal.
//   https://people.cs.vt.edu/ryder/314/f01/lectures/Formal3-2up.pdf
//   https://www.freepascal.org/docs-html/ref/refsu57.html
//   https://stackoverflow.com/q/6911214
// The question at stack overflow has code we can use.

// We don't have IO statements, by design. We can use scanf and printf
//   from libc of the platform. For these purposes there are LibraryLoad,
//   LibraryCall, LibraryUnload built-ins.


// We can disregard program parameters.
//   https://stackoverflow.com/a/74253683
%%
%left "+" "-";
%left "*" "/";

%start CompilationUnit;

CompilationUnit:      ProgramModule EOF {
                          $$ = std::move($1);
                          driver.set_ast(std::move($$));
                      };
ProgramModule:        PROGRAM identifier ProgramParametersOpt ";" Block "." {
                          $$ = pas::ast::ProgramModule(std::move($2), std::move($5));
                      };
ProgramParametersOpt: ProgramParameters | %empty;
ProgramParameters:    "(" IdentList ")";
IdentList:            identifier {
                          // Dunno, why in this case it works and in others it doesn't!
                          $$ = std::vector<std::string>({std::move($1)});
                      }
|                     identifier "," IdentList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };

Block:                Declarations StatementSequence {
                          $$ = pas::ast::Block(std::make_unique<pas::ast::Declarations>(std::move($1)), std::move($2));
                      };
Declarations:         ConstantDefBlockOpt
                      TypeDefBlockOpt
                      VariableDeclBlockOpt
                      SubprogDeclListOpt {
                          $$ = pas::ast::Declarations(std::move($1), std::move($2), std::move($3), std::move($4));
                      }
;
ConstantDefBlockOpt:  ConstantDefBlock {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::ConstDef>();
                      };
TypeDefBlockOpt:      TypeDefBlock {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::TypeDef>();
                      };
VariableDeclBlockOpt: VariableDeclBlock {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::VarDecl>();
                      };
SubprogDeclListOpt:   SubprogDeclList {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::SubprogDecl>();
                      };
ConstantDefBlock:     CONST ConstantDefList {
                          $$ = std::move($2);
                      };
ConstantDefList:      ConstantDef ";" {
                          $$ = std::vector<pas::ast::ConstDef>();
                          $$.emplace_back(std::move($1));
                      }
|                     ConstantDef ";" ConstantDefList {
                          // If profiling will show that this takes time,
                          //   rewrite grammar to insert items to the back
                          //   of the vector. It should be fairly easy.
                          //   Same for all similar cases below.
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
TypeDefBlock:         TYPE TypeDefList {
                          $$ = std::move($2);
                      };
TypeDefList:          TypeDef ";" {
                          // std::vector{...}
                          // This looks for initializer-list constructors as single argument.
                          //   And if there are no such constructors, starts matching once
                          //   again, but looks for constructors with argument list equal
                          //   to items of initializer list.
                          //   https://stackoverflow.com/a/9723448
                          //   So for C-like objects without user-defined constructors,
                          //   it's just C-like field initialization.
                          // I'll use parenthesis, it's more usual for me..
                          $$ = std::vector<pas::ast::TypeDef>();
                          $$.emplace_back(std::move($1));
                      }
|                     TypeDef ";" TypeDefList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      }
VariableDeclBlock:    VAR VariableDeclList {
                          $$ = std::move($2);
                      };
VariableDeclList:     VariableDecl ";" {
                          $$ = std::vector<pas::ast::VarDecl>();
                          $$.emplace_back(std::move($1));
                      }
|                     VariableDecl ";" VariableDeclList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
ConstantDef:          identifier "=" ConstExpression {
                          $$ = pas::ast::ConstDef(std::move($1), std::move($3));
                      };
TypeDef:              identifier "=" Type {
                          $$ = pas::ast::TypeDef(std::move($1), std::move($3));
                      };
VariableDecl:         IdentList  ":" Type {
                          $$ = pas::ast::VarDecl(std::move($1), std::move($3));
                      };

ConstExpression:      UnaryOperatorOpt ConstFactor {
                          $$ = pas::ast::ConstExpr(std::move($1), std::move($2));
                      }
|                     CharacterConst {
                          // TODO: extract range.
                          $$ = pas::ast::ConstExpr();
                          throw "Not implemented";
                      };
UnaryOperatorOpt:     UnaryOperator {
                          $$ = std::optional(std::move($1));
                      }
|                     %empty {
                          $$ = std::optional<pas::ast::UnaryOp>();
                      };
ConstFactor:          identifier {
                          $$ = pas::ast::ConstFactor(std::in_place_type<std::string>, std::move($1));
                      }
|                     number {
                          // Allow int32_t, int64_t, uint64_t.
                          $$ = pas::ast::ConstFactor(std::in_place_type<int>, std::move($1));
                      }
|                     TRUE {
                          $$ = pas::ast::ConstFactor(std::in_place_type<bool>, true);
                      }
|                     FALSE {
                          $$ = pas::ast::ConstFactor(std::in_place_type<bool>, false);
                      }
|                     NIL {
                          $$ = pas::ast::ConstFactor(std::in_place_type<std::monostate>);
                      };
Type:                 identifier {
                          $$ = std::make_unique<pas::ast::NamedType>(std::move($1));
                      }
|                     ArrayType {
                          $$ = std::make_unique<pas::ast::ArrayType>(std::move($1));
                      }
|                     PointerType {
                          $$ = std::make_unique<pas::ast::PointerType>(std::move($1));
                      }
|                     RecordType {
                          $$ = std::make_unique<pas::ast::RecordType>(std::move($1));
                      }
|                     SetType {
                          $$ = std::make_unique<pas::ast::SetType>(std::move($1));
                      };
ArrayType:            ARRAY "[" SubrangeList "]" OF Type {
                          $$ = pas::ast::ArrayType(std::move($3), std::move($6));
                      };
SubrangeList:         Subrange {
                          $$ = std::vector<pas::ast::Subrange>();
                          $$.emplace_back(std::move($1));
                      }
|                     Subrange "," SubrangeList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
Subrange:             ConstFactor ".." ConstFactor {
                          $$ = pas::ast::Subrange(std::move($1), std::move($3));
                      }
|                     CharSubrange {
                          // TODO: figure out, how to store this.
                          // Somehow extract parts. Maybe using regular expressions.
                          throw "Not implemented!";
                      };
RecordType:           RECORD FieldListSequence END {
                          $$ = pas::ast::RecordType(std::move($2));
                      };
SetType:              SET OF Subrange {
                          $$ = pas::ast::SetType(std::move($3));
                      };
PointerType:          "^" identifier {
                          $$ = pas::ast::PointerType(std::move($2));
                      };
FieldListSequence:    FieldList {
                          $$ = std::vector<pas::ast::FieldList>();
                          $$.emplace_back(std::move($1));
                      }
|                     FieldList ";" FieldListSequence {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
FieldList:            IdentList ":" Type {
                          $$ = pas::ast::FieldList(std::move($1), std::move($3));
                      };

StatementSequence:    BEGIN StatementList END {
                          $$ = std::move($2);
                      };
StatementList:        Statement {
                          $$ = std::vector<pas::ast::Stmt>();
                          $$.emplace_back(std::move($1));
                      }
|                     Statement ";" StatementList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
Statement:            Assignment {
                          $$ = std::make_unique<pas::ast::Assignment>(std::move($1));
                      }
|                     ProcedureCall {
                          $$ = std::make_unique<pas::ast::ProcCall>(std::move($1));
                      }
|                     IfStatement {
                          $$ = std::make_unique<pas::ast::IfStmt>(std::move($1));
                      }
|                     CaseStatement {
                          $$ = std::make_unique<pas::ast::CaseStmt>(std::move($1));
                      }
|                     WhileStatement {
                          $$ = std::make_unique<pas::ast::WhileStmt>(std::move($1));
                      }
|                     RepeatStatement {
                          $$ = std::make_unique<pas::ast::RepeatStmt>(std::move($1));
                      }
|                     ForStatement {
                          $$ = std::make_unique<pas::ast::ForStmt>(std::move($1));
                      }
|                     MemoryStatement {
                          $$ = std::make_unique<pas::ast::MemoryStmt>(std::move($1));
                      }
|                     StatementSequence {
                          $$ = std::make_unique<pas::ast::StmtSeq>(std::move($1));
                      }
|                     %empty {
                          $$ = std::make_unique<pas::ast::EmptyStmt>();
                      };
Assignment:           Designator ":=" Expression {
                          $$ = pas::ast::Assignment(std::move($1), std::move($3));
                      };
ProcedureCall:        identifier ActualParametersOpt {
                          $$ = pas::ast::ProcCall(std::move($1), std::move($2));
                      };
ActualParametersOpt:  ActualParameters {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::Expr>();
                      };
IfStatement:          IF Expression THEN Statement                 %prec IFPREC {
                          $$ = pas::ast::IfStmt(std::move($2), std::move($4));
                      }
|                     IF Expression THEN Statement ELSE Statement {
                          $$ = pas::ast::IfStmt(std::move($2), std::move($4), std::move($6));
                      };
CaseStatement:        CASE Expression OF CaseList END {
                          $$ = pas::ast::CaseStmt(std::move($2), std::move($4));
                      };
CaseList:             Case {
                          $$ = std::vector<pas::ast::Case>();
                          $$.emplace_back(std::move($1));
                      }
|                     Case ";" CaseList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
Case:                 CaseLabelList ":" Statement {
                          $$ = pas::ast::Case(std::move($1), std::move($3));
                      };
CaseLabelList:        ConstExpression {
                          $$ = std::vector<pas::ast::ConstExpr>();
                          $$.emplace_back(std::move($1));
                      }
|                     ConstExpression "," CaseLabelList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
WhileStatement:       WHILE Expression DO Statement {
                          $$ = pas::ast::WhileStmt(std::move($2), std::move($4));
                      };
RepeatStatement:      REPEAT StatementSequence UNTIL Expression {
                          $$ = pas::ast::RepeatStmt(std::move($2), std::move($4));
                      };
ForStatement:         FOR identifier ":=" Expression WhichWay Expression DO Statement {
                          $$ = pas::ast::ForStmt(std::move($2), std::move($4), std::move($5), std::move($6), std::move($8));
                      };
WhichWay:             TO {
                          $$ = pas::ast::WhichWay::To;
                      }
|                     DOWNTO {
                          $$ = pas::ast::WhichWay::DownTo;
                      };
Designator:           identifier DesignatorStuffOpt {
                          $$ = pas::ast::Designator(std::move($1), std::move($2));
                      };
DesignatorStuffOpt:   DesignatorStuff {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::DesignatorItem>();
                      };
DesignatorStuff:      DesignatorItem {
                          $$ = std::vector<pas::ast::DesignatorItem>();
                          $$.emplace_back(std::move($1));
                      }
|                     DesignatorItem DesignatorStuff {
                          $$ = std::move($2);
                          $$.insert($$.begin(), std::move($1));
                      };
DesignatorItem:       "." identifier {
                          $$ = pas::ast::DesignatorFieldAccess(std::move($2));
                      }
|                     "[" ExpList "]" {
                          std::vector<std::unique_ptr<pas::ast::Expr>> exprs;
                          for (auto& expr: $2) {
                              exprs.push_back(std::make_unique<pas::ast::Expr>(std::move(expr)));
                          }
                          $$ = pas::ast::DesignatorArrayAccess(std::move(exprs));
                      }
|                     "^" {
                          $$ = pas::ast::DesignatorPointerAccess();
                      };
ActualParameters:     "(" ExpList ")" {
                          $$ = std::move($2);
                      };
ExpList:              Expression {
                          $$ = std::vector<pas::ast::Expr>();
                          $$.emplace_back(std::move($1));
                      }
|                     Expression "," ExpList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
MemoryStatement:      NEW "(" identifier ")" {
                          $$ = pas::ast::MemoryStmt(pas::ast::MemoryStmt::Kind::New, std::move($3));
                      }
|                     DISPOSE "(" identifier ")" {
                          $$ = pas::ast::MemoryStmt(pas::ast::MemoryStmt::Kind::Dispose, std::move($3));
                      };

Expression:           SimpleExpression {
                          // start_expr
                          $$ = pas::ast::Expr(std::move($1));
                      }
|                     SimpleExpression Relation SimpleExpression {
                          // start_expr; Op: rel, rhs_expr
                          $$ = pas::ast::Expr(std::move($1), pas::ast::Expr::Op{std::move($2), std::move($3)});
                      }
;
SimpleExpression:     UnaryOperatorOpt TermList {
                          // unary_operator, start_term; ops: add_operator, term
                          auto& terms = $2.first;
                          auto& ops   = $2.second;
                          pas::ast::Term term = std::move(terms.front());
                          terms.erase(terms.begin());
                          std::vector<pas::ast::SimpleExpr::Op> expr_ops;
                          assert(terms.size() == ops.size());
                          for (size_t i = 0; i < ops.size(); ++i) {
                              expr_ops.emplace_back(pas::ast::SimpleExpr::Op{std::move(ops[i]), std::move(terms[i])});
                          }
                          $$ = pas::ast::SimpleExpr(std::move($1), std::move(term), std::move(expr_ops));
                      };
TermList:             Term {
                          $$ = std::make_pair(std::vector<pas::ast::Term>(), std::vector<pas::ast::AddOp>());
                          $$.first.emplace_back(std::move($1));
                      }
|                     Term AddOperator TermList {
                          $$ = std::move($3);
                          $$.first.insert($$.first.begin(), std::move($1));
                          $$.second.insert($$.second.begin(), std::move($2));
                      };
Term:                 FactorList {
                          // unary_operator, start_term; ops: add_operator, term
                          auto& factors = $1.first;
                          auto& ops   = $1.second;
                          pas::ast::Factor factor = std::move(factors.front());
                          factors.erase(factors.begin());
                          std::vector<pas::ast::Term::Op> term_ops;
                          assert(factors.size() == ops.size());
                          for (size_t i = 0; i < ops.size(); ++i) {
                              term_ops.emplace_back(pas::ast::Term::Op{std::move(ops[i]), std::move(factors[i])});
                          }
                          $$ = pas::ast::Term(std::move(factor), std::move(term_ops));
                      };
FactorList:           Factor {
                          $$ = std::make_pair(std::vector<pas::ast::Factor>(), std::vector<pas::ast::MultOp>());
                          $$.first.emplace_back(std::move($1));
                      }
|                     Factor MultOperator FactorList {
                          $$ = std::move($3);
                          $$.first.insert($$.first.begin(), std::move($1));
                          $$.second.insert($$.second.begin(), std::move($2));
                      };
Factor:               number {
                          $$ = std::move($1);
                      }
|                     string {
                          $$ = std::move($1);
                      }
|                     TRUE {
                          $$ = true;
                      }
|                     FALSE {
                          $$ = false;
                      }
|                     NIL {
                          $$ = std::monostate();
                      }
|                     Designator {
                          $$ = std::move($1);
                      }
|                     "(" Expression ")" {
                          $$ = std::make_unique<pas::ast::Expr>(std::move($2));
                      }
|                     NOT Factor {
                          $$ = std::make_unique<pas::ast::Negation>(std::move($2));
                      }
|                     Setvalue {
                          throw "Not implemented!";
                          // $$ = std::variant<FactorKind::SetValue>(std::move($1));
                      }
|                     FunctionCall {
                          $$ = std::make_unique<pas::ast::FuncCall>(std::move($1));
                      };
Setvalue:             "[" ElementListOpt "]" {
                          $$ = std::move($2);
                      }
ElementListOpt:       ElementList {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::Element>();
                      };
ElementList:          Element {
                          $$ = std::vector<pas::ast::Element>();
                          $$.emplace_back(std::move($1));
                      }
|                     Element "," ElementList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
FunctionCall:         identifier ActualParameters {
                          $$ = pas::ast::FuncCall(std::move($1), std::move($2));
                      };
Element:              ConstExpression {
                          $$ = pas::ast::ConstExpr(std::move($1));
                      }
|                     ConstExpression ".." ConstExpression {
                          $$ = std::pair<pas::ast::ConstExpr, pas::ast::ConstExpr>(std::move($1), std::move($3));
                      };

SubprogDeclList:      SubprogDecl {
                          $$ = std::vector<pas::ast::SubprogDecl>();
                          $$.emplace_back(std::move($1));
                      }
|                     SubprogDecl SubprogDeclList {
                          $$ = std::move($2);
                          $$.insert($$.begin(), std::move($1));
                      };
SubprogDecl:          ProcedureDecl ";" {
                          $$ = pas::ast::ProcDecl(std::move($1));
                      }
|                     FunctionDecl ";" {
                          $$ = pas::ast::FuncDecl(std::move($1));
                      };
ProcedureDecl:        ProcedureHeading ";" Block {
                          $$ = pas::ast::ProcDecl(std::move($1), std::move($3));
                      };
FunctionDecl:         FunctionHeading ":" identifier ";" Block {
                          // Implementation note: pas::ast::FuncDecl { pas::ast::ProcDecl proc_; ...Type return_type_;
                          pas::ast::ProcDecl proc(std::move($1), std::move($5));
                          $$ = pas::ast::FuncDecl(std::move(proc), std::move($3));
                      };
ProcedureHeading:     PROCEDURE identifier FormalParametersOpt {
                          $$ = pas::ast::ProcHeading(std::move($2), std::move($3));
                      };
FormalParametersOpt:  FormalParameters {
                          $$ = std::move($1);
                      }
|                     %empty {
                          $$ = std::vector<pas::ast::FormalParam>();
                      };
FunctionHeading:      FUNCTION identifier FormalParametersOpt {
                          $$ = pas::ast::ProcHeading(std::move($2), std::move($3));
                      };
FormalParameters:     "(" OneFormalParamList ")" {
                          $$ = std::move($2);
                      };
OneFormalParamList:   OneFormalParam {
                          $$ = std::vector<pas::ast::FormalParam>();
                          $$.emplace_back(std::move($1));
                      }
|                     OneFormalParam ";" OneFormalParamList {
                          $$ = std::move($3);
                          $$.insert($$.begin(), std::move($1));
                      };
OneFormalParam:       VAR IdentList ":" identifier {
                          $$ = pas::ast::FormalParam(std::move($2), std::move($4));
                      };

UnaryOperator:        "+" {
                          $$ = pas::ast::UnaryOp::Plus;
                      }
|                     "-" {
                          $$ = pas::ast::UnaryOp::Minus;
                      };
MultOperator:         "*" {
                          $$ = pas::ast::MultOp::Multiply;
                      }
|                     "/" {
                          $$ = pas::ast::MultOp::RealDiv;
                      }
|                     DIV {
                          $$ = pas::ast::MultOp::IntDiv;
                      }
|                     MOD {
                          $$ = pas::ast::MultOp::Modulo;
                      }
|                     AND {
                          $$ = pas::ast::MultOp::And;
                      };
AddOperator:          "+" {
                          $$ = pas::ast::AddOp::Plus;
                      }
|                     "-" {
                          $$ = pas::ast::AddOp::Minus;
                      }
|                     OR {
                          $$ = pas::ast::AddOp::Or;
                      };
Relation:             "=" {
                          $$ = pas::ast::RelOp::Equal;
                      }
|                     "<>" {
                          $$ = pas::ast::RelOp::NotEqual;
                      }
|                     "<" {
                          $$ = pas::ast::RelOp::Less;
                      }
|                     ">" {
                          $$ = pas::ast::RelOp::Greater;
                      }
|                     "<=" {
                          $$ = pas::ast::RelOp::LessEqual;
                      }
|                     ">=" {
                          $$ = pas::ast::RelOp::GreaterEqual;
                      }
|                     IN {
                          $$ = pas::ast::RelOp::In;
                      };
%%

#if 0
// TODO: provide compilation errors at some point, embed error somehow.
assignment:
    "identifier" ":=" exp ";" {
        driver.variables[$1] = $3;
        if (driver.location_debug) {
            std::cerr << driver.location << std::endl;
        }
    }
    | error ";" {
    	// Hint for compilation error, resuming producing messages
    	std::cerr << "You should provide assignment in the form: variable := expression ; " << std::endl;
    };
exp:
    "number"
    | "identifier" {$$ = driver.variables[$1];}
    | exp "+" exp {$$ = $1 + $3; }
    | exp "-" exp {$$ = $1 - $3; }
    | exp "*" exp {$$ = $1 * $3; }
    | exp "/" exp {$$ = $1 / $3; }
    | "(" exp ")" {$$ = $2; };
#endif

void
yy::parser::error(const location_type& l, const std::string& m)
{
  std::cerr << l << ": " << m << '\n';
}
