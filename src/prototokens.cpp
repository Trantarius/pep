#include "parse.hpp"

ProtoToken::operator string(){
    string ret;
    switch(type){
        case NONE:
            return "NONE";
        case COMMA:
            return ",";
        case STATEMENT:
            for(ProtoToken tok:contents){
                ret+=(string)tok+" ";
            }
            return ret+";\n";
        case BLOCK:
            ret+="{\n";
            for(ProtoToken tok:contents){
                ret+="\t";
                ret+=(string)tok;
            }
            return ret+"}";
        case PARENTHESIS:
            ret+="( ";
            for(ProtoToken tok:contents){
                ret+=(string)tok+" ";
            }
            return ret+")";
        case INDEX:
            ret+="[ ";
            for(ProtoToken tok:contents){
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
        else if(source[idx]=='\n'){
            line_col.first++;
            line_col.second=0;
        }
        idx++;
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
        throw parse_error(string("missing ")+(target),start);
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

ProtoToken get_token_of_class(Tracer& trace, charclassfunc_t is_classchar){
    ProtoToken ret;
    long start=trace.idx;
    while(is_classchar(*trace)){
        ++trace;
    }
    ret.source=trace.source.substr(start,trace.idx-start);
    return ret;
}

ProtoToken get_id_token(Tracer& trace){
    ProtoToken ret=get_token_of_class(trace,is_idchar);
    if(ret.source.size()==0){
        return ProtoToken();
    }
    if(list_contains(keywords,ret.source)){
        ret.type=ProtoToken::KEYWORD;
    }else{
        ret.type=ProtoToken::IDENTIFIER;
    }
    return ret;
}
ProtoToken get_num_token(Tracer& trace){
    ProtoToken ret=get_token_of_class(trace,is_numchar);
    if(ret.source.size()==0){
        return ProtoToken();
    }
    ret.type=ProtoToken::NUMBER;
    return ret;
}
ProtoToken get_op_token(Tracer& trace){
    ProtoToken ret=get_token_of_class(trace,is_opchar);
    if(ret.source.size()==0){
        return ProtoToken();
    }
    ret.type=ProtoToken::OPERATOR;
    return ret;
}
ProtoToken get_string_token(Tracer& trace){
    if(*trace!='"'){
        return ProtoToken();
    }
    long start=trace.idx;
    string_seek(trace);
    ProtoToken ret;
    ret.type=ProtoToken::STRING;
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




std::list<ProtoToken> make_prototokens_r(Tracer trace,long stop){
    std::list<ProtoToken> tokens;

    std::list<ProtoToken> running_statement;
    while(trace.idx<stop){

        if(isspace(*trace)){
            ++trace;
            continue;
        }

        if(*trace==';'){
            ProtoToken statement(ProtoToken::STATEMENT,trace.line_col);
            std::swap(statement.contents,running_statement);
            tokens.push_back(statement);
            ++trace;
            continue;
        }

        if(*trace==','){
            ProtoToken tok(ProtoToken::COMMA,trace.line_col);
            running_statement.push_back(tok);
            ++trace;
            continue;
        }

        ProtoToken tok;
        if(isalpha(*trace)||*trace=='_'){
            tok=get_id_token(trace);
            if(tok.type!=ProtoToken::NONE){
                running_statement.push_back(tok);
                continue;
            }
        }

        tok=get_op_token(trace);
        if(tok.type!=ProtoToken::NONE){
            running_statement.push_back(tok);
            continue;
        }

        tok=get_num_token(trace);
        if(tok.type!=ProtoToken::NONE){
            running_statement.push_back(tok);
            continue;
        }

        tok=get_string_token(trace);
        if(tok.type!=ProtoToken::NONE){
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='{'){
            Tracer end=trace;
            seek(end,'}');
            tok=ProtoToken(ProtoToken::BLOCK,trace.line_col);
            ++trace;
            tok.contents=make_prototokens_r(trace,end.idx);
            trace=end;
            ++trace;
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='['){
            Tracer end=trace;
            seek(end,']');
            tok=ProtoToken(ProtoToken::INDEX,trace.line_col);
            ++trace;
            tok.contents=make_prototokens_r(trace,end.idx);
            trace=end;
            ++trace;
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='('){
            Tracer end=trace;
            seek(end,')');
            tok=ProtoToken(ProtoToken::PARENTHESIS,trace.line_col);
            ++trace;
            tok.contents=make_prototokens_r(trace,end.idx);
            trace=end;
            ++trace;
            running_statement.push_back(tok);
            continue;
        }

        if(*trace=='}'||*trace==']'||*trace==')'){
            throw parse_error(string("closing ")+*trace+" without opening bracket",trace.line_col);
        }

        throw parse_error(string("unknown token: ")+(*trace),trace.line_col);
    }

    if(tokens.empty()){
        return running_statement;
    }else if(running_statement.empty()){
        return tokens;
    }else{
        throw parse_error("expected ';'",trace.line_col);
    }
}

std::list<ProtoToken> make_prototokens(const string& code){
    Tracer tracer;
    tracer.source=remove_comments(code);
    return make_prototokens_r(tracer,tracer.source.size());
}
