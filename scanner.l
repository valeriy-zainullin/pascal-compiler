%{
    #include <cerrno>
    #include <climits>
    #include <cstdlib>
    #include <cstring> // strerror
    #include <string>
    #include <iostream>
    #include "driver.hh"
    #include "parser.hh"
%}

%option noyywrap nounput noinput batch debug

%option c++
%option yyclass="Scanner"

%{
  // Code definitions at the end of scanner.cpp

  // A number symbol corresponding to the value in S.
  yy::parser::symbol_type make_number(
    const std::string &s,
    const yy::parser::location_type& loc
  );

  void Scanner::UpdateLocation() {
    if (driver.location_debug) {
        std::cerr << "Action called " << driver.location << std::endl;
    }
    driver.location.columns(yyleng);
  }
%}


id     [a-zA-Z][a-zA-Z_0-9]*
int    [0-9]+
blank  [ \t\r]
/* String escaping is not supported for now..
     https://www.freepascal.org/docs-html/ref/refse8.html
     At some point it will be.
*/
string \"[^"]*\"|\'[^']\'

/* TODO: implement comments. Just match them, log and throw away, accounting newlines inside of them. */

%{
  // Code run each time a pattern is matched.
  #define YY_USER_ACTION UpdateLocation();
%}

%%


%{
  // A handy shortcut to the location held by the driver.
  yy::location& loc = driver.location;
  if (driver.location_debug) {
  // Code run each time yylex is called.
    std::cerr << "BEFORE " << loc << std::endl;
  }
  // loc.step();
  if (driver.location_debug) {
    std::cerr << "AFTER " <<  loc << std::endl;
  }
%}

{blank}+   {
    if (driver.location_debug) {
        std::cerr << "Blank matched" << std::endl;
    }
    // loc.step();
}

\n+ {
    if (driver.location_debug) {
        std::cerr << "EOL called" << std::endl;
    }
    loc.lines(yyleng);
    loc.step();
}

"-"         {
               return yy::parser::make_MINUS  (loc);
            }
"+"         return yy::parser::make_PLUS      (loc);
"*"         return yy::parser::make_STAR      (loc);
"/"         return yy::parser::make_SLASH     (loc);
"("         return yy::parser::make_LPAREN    (loc);
")"         return yy::parser::make_RPAREN    (loc);
":="        return yy::parser::make_ASSIGN    (loc);
"="         return yy::parser::make_EQ        (loc);
"<>"        return yy::parser::make_NEQ       (loc);
"<"         return yy::parser::make_LT        (loc);
">"         return yy::parser::make_GT        (loc);
"<="        return yy::parser::make_LEQ       (loc);
">="        return yy::parser::make_GEQ       (loc);
"*"         return yy::parser::make_STAR      (loc);
"/"         return yy::parser::make_SLASH     (loc); 
"("         return yy::parser::make_LPAREN    (loc);
")"         return yy::parser::make_RPAREN    (loc);
"["         return yy::parser::make_LBRACKET  (loc);
"]"         return yy::parser::make_RBRACKET  (loc);
"."         return yy::parser::make_DOT       (loc);
","         return yy::parser::make_COMMA     (loc);
":"         return yy::parser::make_COLON     (loc);
";"         return yy::parser::make_SEMICOLON (loc);

"div"       return yy::parser::make_DIV       (loc);
"mod"       return yy::parser::make_MOD       (loc);
"in"        return yy::parser::make_IN        (loc);
"not"       return yy::parser::make_NOT       (loc);
"or"        return yy::parser::make_OR        (loc);
"and"       return yy::parser::make_AND       (loc);
 
"array"     return yy::parser::make_ARRAY     (loc);
"begin"     return yy::parser::make_BEGIN     (loc);
"case"      return yy::parser::make_CASE      (loc);
"const"     return yy::parser::make_CONST     (loc);
"do"        return yy::parser::make_DO        (loc);
"downto"    return yy::parser::make_DOWNTO    (loc);
"else"      return yy::parser::make_ELSE      (loc);
"end"       return yy::parser::make_END       (loc);
"file"      return yy::parser::make_FILE      (loc);
"for"       return yy::parser::make_FOR       (loc);
"function"  return yy::parser::make_FUNCTION  (loc);
"if"        return yy::parser::make_IF        (loc);
"nil"       return yy::parser::make_NIL       (loc);
"of"        return yy::parser::make_OF        (loc);
"packed"    return yy::parser::make_PACKED    (loc);
"procedure" return yy::parser::make_PROCEDURE (loc);
"program"   return yy::parser::make_PROGRAM   (loc);
"record"    return yy::parser::make_RECORD    (loc);
"repeat"    return yy::parser::make_REPEAT    (loc);
"set"       return yy::parser::make_SET       (loc);
"then"      return yy::parser::make_THEN      (loc);
"to"        return yy::parser::make_TO        (loc);
"type"      return yy::parser::make_TYPE      (loc);
"until"     return yy::parser::make_UNTIL     (loc);
"var"       return yy::parser::make_VAR       (loc);
"while"     return yy::parser::make_WHILE     (loc);
"with"      return yy::parser::make_WITH      (loc);

"False"     return yy::parser::make_FALSE     (loc);
"True"      return yy::parser::make_TRUE      (loc);

"New"       return yy::parser::make_NEW       (loc);
"Dispose"   return yy::parser::make_DISPOSE   (loc);


{int}       return make_number(yytext, loc);
{string}    return yy::parser::make_string(yytext, loc);
{id}       {
                if (driver.location_debug) {
                    std::cerr << "ID found " << yytext << std::endl;
                }
                return yy::parser::make_identifier(yytext, loc);
           }
.          {
                throw yy::parser::syntax_error(loc, "invalid character: " + std::string(yytext));
           }
<<EOF>>    return yy::parser::make_EOF(loc);
%%

yy::parser::symbol_type make_number(
  const std::string &s,
  const yy::parser::location_type& loc
) {
  errno = 0;
  long n = strtol(s.c_str(), NULL, 10);
  if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE))
    throw yy::parser::syntax_error(loc, "integer is out of range: " + s);
  return yy::parser::make_number((int) n, loc);
}
