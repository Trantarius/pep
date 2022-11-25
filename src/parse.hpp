#include "CUtil/util.hpp"
#include <stdexcept>

typedef std::pair<long,long> line_col_t;

/*
 * throwable error which has a line/column of the error location in the source
 */
class parse_error:public std::runtime_error{
    string compose_message(const string& message,line_col_t line_col){
        return message+" "+tostr(line_col.first)+":"+tostr(line_col.second);
    }
public:
    const line_col_t line_col;

    parse_error(const string& message,line_col_t line_col):
        std::runtime_error(compose_message(message,line_col)),line_col(line_col){}
};

struct Token{
    enum Type{NONE=0, COMMA, STATEMENT, BLOCK, PARENTHESIS, INDEX, IDENTIFIER, NUMBER, STRING, KEYWORD, OPERATOR} type=NONE;

    std::list<Token> contents;
    string source;
    line_col_t line_col;

    //Token(){}
    Token(Type type=NONE,line_col_t line_col=line_col_t{0,0}):
        type(type),line_col(line_col){}

    operator string();
};

void parse(string path);
