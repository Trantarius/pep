#include "parse.hpp"
#include <list>
#include <cctype>
/*
const std::list<std::pair<char,char>> brackets{
    std::pair{'(',')'},
    std::pair{'[',']'},
    std::pair{'{','}'}
};
*/

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
    LESS, GREATER, LESS_EQ, GREATER_EQ
};










Token::operator string(){
    string ret;
    switch(type){
        case NONE:
            return "NONE";
        case COMMA:
            return ",";
        case STATEMENT:
            for(Token tok:contents){
                ret+=(string)tok+" ";
            }
            return ret+";\n";
        case BLOCK:
            ret+="{\n";
            for(Token tok:contents){
                ret+="\t";
                ret+=(string)tok;
            }
            return ret+"}";
        case PARENTHESIS:
            ret+="( ";
            for(Token tok:contents){
                ret+=(string)tok+" ";
            }
            return ret+")";
        case INDEX:
            ret+="[ ";
            for(Token tok:contents){
                ret+=(string)tok+" ";
            }
            return ret+"]";
        case IDENTIFIER:
        case NUMBER:
        case STRING:
        case KEYWORD:
        case OPERATOR:
            return source;
    }
}



struct Tracer{
    string source;
    long idx=0;
    line_col_t line_col{0,0};

    long length(){
        return source.length();
    }

    void next(){
        if(idx>=source.length()){
            throw std::out_of_range("Tracer past end of source");
        }
        if(isprint(source[idx])){
            line_col.second++;
        }
        idx++;
        if(source[idx]=='\n'){
            line_col.first++;
            line_col.second=0;
        }
    }

    Tracer& operator++(){//prefix
        next();
        return *this;
    }
    char operator*(){
        return source[idx];
    }
};




template<typename T>
bool list_contains(const std::list<T>& list,const T& item){
    for(auto it=list.begin();it!=list.end();it++){
        if(*it==item){
            return true;
        }
    }
    return false;
}
/*
std::pair<long,long> get_line_col(const string& source, long idx){
    long line=0,col=0;
    for(long n=0;n<idx;n++){
        if(!isprint(source[n])){
            continue;
        }
        col++;
        if(source[n]=='\n'){
            line++;
            col=0;
        }
    }
    return std::pair{line,col};
}

string get_line_col_str(const string& source, long idx){
    std::pair<long,long> line_col=get_line_col(source,idx);
    return tostr(line_col.first)+":"+tostr(line_col.second);
}
*/

/*
 * seeks a quotation mark, skipping escaped characters.
 * does not check current character.
 */
void string_seek(Tracer& trace){
    line_col_t start=trace.line_col;
    try{
        while(*(++trace)!='"'){
            if(*trace=='\\'){
                ++trace;
            }
        }
    }catch(std::out_of_range err){
        throw parse_error("unclosed string",start);
    }
}
/*
 * moves idx to the next instance of "target", skipping sections inside parentheses and strings.
 * does not check current character.
 */
void seek(Tracer& trace,char target){
    line_col_t start=trace.line_col;
    try{
        while(*(++trace)!=target){
            switch(*trace){
                case '\\':
                    ++trace;
                    continue;
                case '"':
                    string_seek(trace);
                    continue;
                case '(':
                    seek(trace,')');
                    continue;
                case '{':
                    seek(trace,'}');
                    continue;
                case '[':
                    seek(trace,']');
                    continue;
            }
        }
    }catch(std::out_of_range err){
        throw parse_error("missing "+std::to_string(target),start);
    }
}







typedef bool(*charclassfunc_t)(char);

bool is_numchar(char c){
    return isdigit(c) || c=='.' || c=='_';
}
bool is_idchar(char c){
    return isalnum(c) || c=='_';
}
bool is_opchar(char c){
    for(auto it=operatorchars.begin();it!=operatorchars.end();it++){
        if(*it==c){
            return true;
        }
    }
    return false;
}

Token get_token_of_class(Tracer& trace, charclassfunc_t is_classchar){
    Token ret;
    long start=trace.idx;
    while(is_classchar(*trace)){
        ++trace;
    }
    ret.source=trace.source.substr(start,trace.idx-start);
    return ret;
}

Token get_id_token(Tracer& trace){
    Token ret=get_token_of_class(trace,is_idchar);
    if(ret.source.size()==0){
        return Token();
    }
    if(list_contains(keywords,ret.source)){
        ret.type=Token::KEYWORD;
    }else{
        ret.type=Token::IDENTIFIER;
    }
    return ret;
}
Token get_num_token(Tracer& trace){
    Token ret=get_token_of_class(trace,is_numchar);
    if(ret.source.size()==0){
        return Token();
    }
    ret.type=Token::NUMBER;
    return ret;
}
Token get_op_token(Tracer& trace){
    Token ret=get_token_of_class(trace,is_opchar);
    if(ret.source.size()==0){
        return Token();
    }
    ret.type=Token::OPERATOR;
    return ret;
}
Token get_string_token(Tracer& trace){
    if(*trace!='"'){
        return Token();
    }
    long start=trace.idx;
    string_seek(trace);
    Token ret;
    ret.type=Token::STRING;
    ret.source=trace.source.substr(start,trace.idx-start+1);
    ++trace;
    return ret;
}






/*
 * creates a copy of the given string with any commented parts removed. newlines will be
 * preserved, so that the line numbers are correct for non-commented sections
 */
string remove_comments(const string& source){
    char* ret=new char[source.length()+1]{};
    bool in_line_comment=false;
    bool in_block_comment=false;
    long ridx=0;
    line_col_t last_entered_block_at{-1,-1};
    Tracer trace;
    trace.source=std::move(source);
    for(;trace.idx<trace.length();++trace){
        if(in_line_comment){
            if(*trace=='\n'){
                in_line_comment=false;
                ret[ridx++]='\n';
            }
        }else if(*trace==commentchar){
            if(trace.source[trace.idx+1]==commentchar){
                in_block_comment=!in_block_comment;
                last_entered_block_at=trace.line_col;
                ++trace;
            }else{
                in_line_comment=true;
            }
        }else if(!in_block_comment || *trace=='\n'){
            ret[ridx++]=*trace;
        }
    }
    if(in_block_comment){
        throw parse_error("unclosed block comment",last_entered_block_at);
    }
    string retstr(ret,ridx);
    delete [] ret;
    return retstr;
}




std::list<Token> make_tokens(Tracer trace,long stop){
    std::list<Token> tokens;

    std::list<Token> running_statement;
    while(trace.idx<stop){

        if(isspace(*trace)){
            ++trace;
            continue;
        }

        if(*trace==';'){
            Token statement(Token::STATEMENT,trace.line_col);
            std::swap(statement.contents,running_statement);
            tokens.push_back(statement);
            ++trace;
            continue;
        }

        if(*trace==','){
            Token tok(Token::COMMA,trace.line_col);
            running_statement.push_back(tok);
            ++trace;
            continue;
        }

        Token tok;
        if(isalpha(*trace)||*trace=='_'){
            tok=get_id_token(trace);
            if(tok.type!=Token::NONE){
                running_statement.push_back(tok);
                continue;
            }
        }

        tok=get_op_token(trace);
        if(tok.type!=Token::NONE){
            running_statement.push_back(tok);
            continue;
        }

        tok=get_num_token(trace);
        if(tok.type!=Token::NONE){
            running_statement.push_back(tok);
            continue;
        }

        tok=get_string_token(trace);
        if(tok.type!=Token::NONE){
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='{'){
            Tracer end=trace;
            seek(end,'}');
            tok=Token(Token::BLOCK,trace.line_col);
            ++trace;
            tok.contents=make_tokens(trace,end.idx);
            trace=end;
            ++trace;
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='['){
            Tracer end=trace;
            seek(end,']');
            tok=Token(Token::INDEX,trace.line_col);
            ++trace;
            tok.contents=make_tokens(trace,end.idx);
            trace=end;
            ++trace;
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='('){
            Tracer end=trace;
            seek(end,')');
            tok=Token(Token::PARENTHESIS,trace.line_col);
            ++trace;
            tok.contents=make_tokens(trace,end.idx);
            trace=end;
            ++trace;
            running_statement.push_back(tok);
            continue;
        }

        throw parse_error(string("unknown token:")+(*trace),trace.line_col);
    }

    if(tokens.empty()){
        return running_statement;
    }else if(running_statement.empty()){
        return tokens;
    }else{
        throw parse_error("expected ';'",trace.line_col);
    }
}

std::list<Token> tokenize(const string& code){
    Tracer tracer;
    tracer.source=remove_comments(code);
    return make_tokens(tracer,tracer.source.size());
}



void parse(string path){
    bloc sourcebloc=readfile(path);
    string source((char*)sourcebloc.ptr,sourcebloc.size);\
    sourcebloc.destroy();
    try{
        std::list<Token> tokens=tokenize(source);
        for(Token tok:tokens){
            std::cout<<(string)tok;
        }
        std::cout<<std::endl;
    }catch(parse_error err){
        printerr("parse error: ",err.what()," in ",path);
    }
}
