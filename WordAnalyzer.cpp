#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cctype>
using namespace std;

struct Token {
    string type;
    string text;
};

string input;
size_t pos = 0;
int len;

bool is_ident_start(char c){ return (c=='_'||isalpha((unsigned char)c)); }
bool is_ident_char(char c){ return (c=='_'||isalnum((unsigned char)c)); }

void skip_whitespace(){
    while(pos < input.size()){
        char c = input[pos];
        if(c==' '||c=='\t' || c=='\n' || c=='\r') { pos++; continue; }
        if(c=='/' && pos+1 < input.size()){
            if(input[pos+1]=='/'){
                pos += 2;
                while(pos < input.size() && input[pos] != '\n') pos++;
                continue;
            } else if(input[pos+1]=='*'){
                pos += 2;
                while(pos+1 < input.size() && !(input[pos]=='*' && input[pos+1]=='/')) pos++;
                if(pos+1 < input.size()) pos += 2;
                continue;
            }
        }
        break;
    }
}

string escape_for_output(const string &s){
    string out;
    for(char c: s){
        if(c=='\\') out += "\\\\";
        else if(c=='\"') out += "\\\"";
        else out.push_back(c);
    }
    return out;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // read all stdin
    {
        std::ostringstream ss;
        ss << cin.rdbuf();
        input = ss.str();
    }
    len = input.size();

    unordered_set<string> keywords = {"int","void","if","else","while","break","continue","return"};

    vector<Token> tokens;

    auto peek = [&](size_t off=0)->char{ return (pos+off < input.size()) ? input[pos+off] : '\0'; };

    auto prev_is_value = [&](const Token* prev)->bool{
        if(!prev) return false;
        if(prev->type=="Ident" || prev->type=="IntConst" ) return true;
        if(prev->type.size()>=1 && prev->type[0]=='\'' && prev->text==")") return true;
        return false;
    };

    while(true){
        skip_whitespace();
        if(pos >= input.size()) break;

        Token tk;
        char c = peek();

        if(is_ident_start(c)){
            size_t st = pos;
            pos++;
            while(pos < input.size() && is_ident_char(input[pos])) pos++;
            string s = input.substr(st, pos-st);
            if(keywords.count(s)){
                tk.type = "'" + s + "'"; 
                tk.text = s;
            } else {
                tk.type = "Ident";
                tk.text = s;
            }
            tokens.push_back(tk);
            continue;
        }

        if (isdigit(c)) {
            size_t st = pos;
            while (isdigit(peek())) pos++;
            string s = input.substr(st, pos - st);
            tk.type = "IntConst";
            tk.text = s;
            tokens.push_back(tk);
            continue;
        }


        if(c=='|' && peek(1)=='|'){
            tk.type = "'||'"; tk.text = "||"; pos+=2; tokens.push_back(tk); continue;
        }
        if(c=='&' && peek(1)=='&'){
            tk.type = "'&&'"; tk.text = "&&"; pos+=2; tokens.push_back(tk); continue;
        }
        if(c=='=' && peek(1)=='='){
            tk.type = "'=='"; tk.text = "=="; pos+=2; tokens.push_back(tk); continue;
        }
        if(c=='!' && peek(1)=='='){
            tk.type = "'!='"; tk.text = "!="; pos+=2; tokens.push_back(tk); continue;
        }
        if(c=='<' && peek(1)=='='){
            tk.type = "'<='"; tk.text = "<="; pos+=2; tokens.push_back(tk); continue;
        }
        if(c=='>' && peek(1)=='='){
            tk.type = "'>='"; tk.text = ">="; pos+=2; tokens.push_back(tk); continue;
        }

        string one(1,c);
        vector<char> single_ops = {'+', '-', '*', '/', '%', '<', '>', '=', ';', ',', '(', ')', '{', '}', '!'};
        if(find(single_ops.begin(), single_ops.end(), c) != single_ops.end()){
            tk.type = string("'")+one+string("'");
            tk.text = one;
            pos++;
            tokens.push_back(tk);
            continue;
        }

        pos++;
    }

    for(size_t i=0;i<tokens.size();++i){
        string type = tokens[i].type;
        string text = tokens[i].text;
       
        cout << i << ":" << type << ":\"" << escape_for_output(text) << "\"";
        if(i+1 < tokens.size()) cout << '\n';
    }

    return 0;
}
