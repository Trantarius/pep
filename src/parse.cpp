#include "parse.hpp"
#include <list>
#include <cctype>







std::list<ProtoToken> tokenize(const string& code){
    return make_prototokens(code);
}



void parse(string path){
    bloc sourcebloc=readfile(path);
    string source((char*)sourcebloc.ptr,sourcebloc.size);
    sourcebloc.destroy();
    try{
        std::list<ProtoToken> tokens=tokenize(source);
        for(ProtoToken tok:tokens){
            std::cout<<(string)tok;
        }
        std::cout<<std::endl;
    }catch(parse_error err){
        printerr("parse error: ",err.what()," in ",path);
    }
}
