#include "CUtil/util.hpp"
#include <stdexcept>

typedef std::pair<long,long> line_col_t;


const std::list<string> keywords{
    "import",
    "for","in",
    "func","return",
    "true","false"
};

const std::list<string> peptypes{
    "int8","uint8",
    "int16","uint16",
    "int32","uint32",
    "int64","uint64",
    "int","uint",

    "float32","float64","float",
    "void",
    "string",
    "bool"
};

const string operatorchars="+-*/=!&|^%><.:~";

const char commentchar='#';

enum Operators{
    ASSIGN,
    ADD,SUB,MUL,DIV,MOD,AND,OR,XOR,NOT,
    ADD_EQ, SUB_EQ, MUL_EQ, DIV_EQ, MOD_EQ, AND_EQ, OR_EQ, XOR_EQ, NOT_EQ,
    OBJ_MEM, SPACE_MEM,
    LESS, GREATER, LESS_EQ, GREATER_EQ,
    ELLIPSIS, ARROW
};

const char OperatorStrings[][4]{
    //ASSIGN
    "=",
    //ADD
    "+",
    //SUB
    "-",
    //MUL
    "*",
    //DIV
    "/",
    //MOD
    "%",
    //AND
    "&",
    //OR
    "|",
    //XOR
    "^",
    //NOT
    "!",
    //ADD_EQ
    "+=",
    //SUB_EQ
    "-=",
    //MUL_EQ
    "*=",
    //DIV_EQ
    "/=",
    //MOD_EQ
    "%=",
    //AND_EQ
    "&=",
    //OR_EQ
    "|=",
    //XOR_EQ
    "^=",
    //NOT_EQ
    "!=",
    //OBJ_MEM
    ".",
    //SPACE_MEM
    ":",
    //LESS
    "<",
    //GREATER
    ">",
    //LESS_EQ
    "<=",
    //GREATER_EQ
    ">=",
    //ELLIPSIS
    "...",
    //ARROW
    "->"
};


typedef bloc<char,true> blocstr;


/*
 * throwable error which has a line/column of the error location in the source
 */
class parse_error:public std::runtime_error{
    string compose_message(const string& message,line_col_t line_col){
        return message+" "+tostr(line_col.first+1)+":"+tostr(line_col.second+1);
    }
public:
    const line_col_t line_col;

    parse_error(const string& message,line_col_t line_col):
        std::runtime_error(compose_message(message,line_col)),line_col(line_col){}
};

struct ProtoToken{
    enum Type{NONE=0, COMMA, STATEMENT, BLOCK, PARENTHESIS, INDEX, IDENTIFIER, NUMBER, STRING, KEYWORD, OPERATOR} type=NONE;

    std::list<ProtoToken> contents;
    string source;
    line_col_t line_col;

    //Token(){}
    ProtoToken(Type type=NONE,line_col_t line_col=line_col_t{0,0}):
        type(type),line_col(line_col){}

    operator string();
};

std::list<ProtoToken> make_prototokens(const string& code);

void parse(string path);
