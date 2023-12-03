%skeleton "lalr1.cc"
%require "3.5"

%defines
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <string>
    /* Forward declaration of classes in order to disable cyclic dependencies */
    class Scanner;
    class Driver;
}


%define parse.trace
%define parse.error verbose

%code {
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
%token <std::string> string "string"
%token <int> number "number"

%token <std::pair<char, char>> CharSubrange   // 'a..z', no multibyte for now (and wide chars).
%token <char>                  CharacterConst // 'a', no multibyte characters for now.

// Prints output in parsing option for debugging location terminal
%printer { print_token(yyo, $$); } <*>;

// Dangling else problem in pascal.
//   https://people.cs.vt.edu/ryder/314/f01/lectures/Formal3-2up.pdf
//   https://www.freepascal.org/docs-html/ref/refsu57.html
//   https://stackoverflow.com/q/6911214
// The question at stack overflow has code we can use.

// We don't have IO statements, by design. We can use scanf and printf
//   from libc of the platform. For these purposes there are LibraryLoad,
//   LibraryCall, LibraryUnload built-ins.

%%
%left "+" "-";
%left "*" "/";

%start CompilationUnit;

CompilationUnit:      ProgramModule EOF;
ProgramModule:        PROGRAM identifier ProgramParameters ';' Block '.';
ProgramParameters:    '(' IdentList ')';
IdentList:            identifier | identifier ',' IdentList;

Block:                DeclarationsOpt StatementSequence;
DeclarationsOpt:      Declarations | %empty;
Declarations:         ConstantDefBlockOpt
|                     TypeDefBlockOpt
|                     VariableDeclBlockOpt
|                     SubprogDeclListOpt
;
ConstantDefBlockOpt:  ConstantDefBlock  | %empty;
TypeDefBlockOpt:      TypeDefBlock      | %empty;
VariableDeclBlockOpt: VariableDeclBlock | %empty;
SubprogDeclListOpt:   SubprogDeclList   | %empty;
ConstantDefList:      ConstantDef ';'  | ConstantDef ';' ConstantDefList;
ConstantDefBlock:     CONST ConstantDefList;
TypeDefList:          TypeDef ';'      | TypeDef ';' TypeDefList;
TypeDefBlock:         TYPE TypeDefList;
VariableDeclList:     VariableDecl ';' | VariableDecl ';' VariableDeclList;
VariableDeclBlock:    VAR VariableDeclList;
ConstantDef:          identifier '=' ConstExpression;
TypeDef:              identifier '=' Type;
VariableDecl:         IdentList  ':' Type;

ConstExpression:      UnaryOperatorOpt ConstFactor | CharacterConst | NIL;
UnaryOperatorOpt:     UnaryOperator | %empty;
ConstFactor:          identifier | number | TRUE | FALSE | NIL;
Type:                 identifier | ArrayType | PointerType | RecordType | SetType;
SubrangeList:         Subrange | Subrange ',' SubrangeList;
ArrayType:            ARRAY '[' SubrangeList ']' OF Type;
Subrange:             ConstFactor ".." ConstFactor | CharSubrange;
RecordType:           RECORD FieldListSequence END;
SetType:              SET OF Subrange;
PointerType:          '^' identifier;
FieldListSequence:    FieldList | FieldList ';' FieldList;
FieldList:            IdentList ':' Type;

StatementSequence:    BEGIN StatementList END;
StatementList:        Statement | Statement ';' StatementList;
Statement:            Assignment | ProcedureCall | IfStatement | CaseStatement | WhileStatement | RepeatStatement | ForStatement | MemoryStatement | StatementSequence | %empty;
Assignment:           Designator ":=" Expression;
ProcedureCall:        identifier ActualParametersOpt;
ActualParametersOpt:  ActualParameters | %empty;
IfStatement:          IF Expression THEN Statement
|                     IF Expression THEN Statement ELSE Statement;
CaseStatement:        CASE Expression OF CaseList END;
CaseList:             Case | Case ';' CaseList;
Case:                 CaseLabelList ':' Statement;
CaseLabelList:        ConstExpression | ConstExpression ',' CaseLabelList;
WhileStatement:       WHILE Expression DO Statement;
RepeatStatement:      REPEAT StatementSequence UNTIL Expression;
ForStatement:         FOR identifier ":=" Expression WhichWay Expression DO Statement;
WhichWay:             TO | DOWNTO;
DesignatorList:       Designator | Designator ',' DesignatorList;
Designator:           identifier DesignatorStuffOpt;
DesignatorStuffOpt:   DesignatorStuff | %empty;
DesignatorStuff:      DesignatorItem | DesignatorItem DesignatorStuff;
DesignatorItem:       '.' identifier | '[' ExpList ']' | '^';
ActualParameters:     '(' ExpList ')';
ExpList:              Expression | Expression ',' ExpList;
MemoryStatement:      NEW '(' identifier ')' | DISPOSE '(' identifier ')';

Expression:           SimpleExpression | SimpleExpression Relation SimpleExpression;
SimpleExpression:     UnaryOperatorOpt TermList;
UnaryOperatorOpt:     UnaryOperator | %empty;
TermList:             Term | Term AddOperator TermList;
Term:                 Factor | Factor MultOperator Term;
Factor:               number | string | TRUE | FALSE | NIL | Designator | '(' Expression ')' | NOT Factor | Setvalue | FunctionCall;
Setvalue:             '[' ElementListOpt ']'
ElementList:          Element | Element ',' ElementList;
ElementListOpt:       %empty | ElementList;
FunctionCall:         identifier ActualParameters;
Element:              ConstExpression | ConstExpression ".." ConstExpression;

SubprogDeclList:      SubprogDecl | SubprogDecl SubprogDeclList;
SubprogDecl:          ProcedureDecl ';' | FunctionDecl ';';
ProcedureDecl:        ProcedureHeading ';' Block;
FunctionDecl:         FunctionHeading ':' identifier ';' Block;
ProcedureHeading:     PROCEDURE identifier FormalParametersOpt;
FormalParametersOpt:  FormalParameters | %empty;
FunctionHeading:      FUNCTION identifier FormalParametersOpt;
FormalParameters:     '(' OneFormalParamList ')';
OneFormalParamList:   OneFormalParam | OneFormalParam ';' OneFormalParamList;
OneFormalParam:       VAR IdentList ':' identifier;

UnaryOperator:       '+' | '-';
MultOperator:        '*' | '/'  | DIV | MOD | AND;
AddOperator:         '+' | '-'  | OR;
Relation:            '=' | "<>" | '<' | '>' | "<=" | ">=" | IN;
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
