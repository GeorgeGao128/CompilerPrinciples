#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <map>
#include <unordered_map>
#include <set>
#include <algorithm>

using namespace std;

class ErrorReporter {
private:
    map<int, string> lineToMessage;
public:
    void report(int line, const string& message) {
        if (lineToMessage.find(line) == lineToMessage.end()) {
            lineToMessage[line] = message;
        }
    }
    bool hasError() const {
        return !lineToMessage.empty();
    }
    const map<int, string>& errors() const {
        return lineToMessage;
    }
};

enum class TokenKind {
    Eof,
    Int, Void, If, Else, While,
    Break, Continue, Return,
    Id, Number,
    Plus, Minus, Star, Div, Mod,
    Lt, Le, Gt, Ge, Eq, Ne,
    And, Or, Not,
    Assign,
    LParen, RParen, LBrace, RBrace,
    Semicolon, Comma
};

struct Token {
    TokenKind type;
    string value;
    int line;
};

class Lexer {
private:
    string input;
    size_t pos;
    int line;
    ErrorReporter& reporter;

    char peek(int offset = 0) {
        if (pos + offset >= input.length()) return '\0';
        return input[pos + offset];
    }

    char advance() {
        if (pos >= input.length()) return '\0';
        char ch = input[pos++];
        if (ch == '\n') line++;
        return ch;
    }

    void skipWhitespace() {
        while (isspace(peek())) {
            advance();
        }
    }

    bool skipComment() {
        if (peek() == '/' && peek(1) == '/') {
            while (peek() != '\n' && peek() != '\0') advance();
            return true;
        }
        if (peek() == '/' && peek(1) == '*') {
            int startLine = line;
            advance(); advance();
            while (true) {
                if (peek() == '\0') {
                    reporter.report(startLine, "Unterminated comment");
                    return false;
                }
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
            return true;
        }
        return false;
    }

public:
    Lexer(const string& src, ErrorReporter& reporterRef) : input(src), pos(0), line(1), reporter(reporterRef) {}

    Token nextToken() {
        static const unordered_map<string, TokenKind> keywordToKind = {
            {"int", TokenKind::Int}, {"void", TokenKind::Void},
            {"if", TokenKind::If}, {"else", TokenKind::Else},
            {"while", TokenKind::While}, {"break", TokenKind::Break},
            {"continue", TokenKind::Continue}, {"return", TokenKind::Return}
        };
        auto isIdentStart = [](char c) { return isalpha(static_cast<unsigned char>(c)) || c == '_'; };
        auto isIdentCont = [](char c) { return isalnum(static_cast<unsigned char>(c)) || c == '_'; };
        while (true) {
            skipWhitespace();
            if (!skipComment()) break;
        }

        Token tok;
        tok.line = line;

        if (peek() == '\0') {
            tok.type = TokenKind::Eof;
            return tok;
        }

        if (isIdentStart(peek())) {
            string id;
            while (isIdentCont(peek())) {
                id += advance();
            }
            tok.value = id;
            auto it = keywordToKind.find(id);
            tok.type = (it != keywordToKind.end()) ? it->second : TokenKind::Id;
            return tok;
        }

        if (isdigit(peek())) {
            string num;
            while (isdigit(peek())) {
                num += advance();
            }
            tok.type = TokenKind::Number;
            tok.value = num;
            return tok;
        }

        char ch = peek();
        switch (ch) {
            case '+': advance(); tok.type = TokenKind::Plus; return tok;
            case '-': advance(); tok.type = TokenKind::Minus; return tok;
            case '*': advance(); tok.type = TokenKind::Star; return tok;
            case '/': advance(); tok.type = TokenKind::Div; return tok;
            case '%': advance(); tok.type = TokenKind::Mod; return tok;
            case '(': advance(); tok.type = TokenKind::LParen; return tok;
            case ')': advance(); tok.type = TokenKind::RParen; return tok;
            case '{': advance(); tok.type = TokenKind::LBrace; return tok;
            case '}': advance(); tok.type = TokenKind::RBrace; return tok;
            case ';': advance(); tok.type = TokenKind::Semicolon; return tok;
            case ',': advance(); tok.type = TokenKind::Comma; return tok;
            case '<':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TokenKind::Le;
                } else {
                    tok.type = TokenKind::Lt;
                }
                return tok;
            case '>':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TokenKind::Ge;
                } else {
                    tok.type = TokenKind::Gt;
                }
                return tok;
            case '=':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TokenKind::Eq;
                } else {
                    tok.type = TokenKind::Assign;
                }
                return tok;
            case '!':
                advance();
                if (peek() == '=') {
                    advance();
                    tok.type = TokenKind::Ne;
                } else {
                    tok.type = TokenKind::Not;
                }
                return tok;
            case '&':
                advance();
                if (peek() == '&') {
                    advance();
                    tok.type = TokenKind::And;
                    return tok;
                }
                break;
            case '|':
                advance();
                if (peek() == '|') {
                    advance();
                    tok.type = TokenKind::Or;
                    return tok;
                }
                break;
        }
        
        advance();
        tok.type = TokenKind::Eof;
        return tok;
    }
};

class TokenStream {
private:
    const vector<Token>& tokens;
    size_t index;
public:
    TokenStream(const vector<Token>& toks) : tokens(toks), index(0) {}
    const Token& current() const {
        if (index >= tokens.size()) return tokens.back();
        return tokens[index];
    }
    const Token& peek(int offset = 0) const {
        if (index + offset >= tokens.size()) return tokens.back();
        return tokens[index + offset];
    }
    void advance() {
        if (index < tokens.size()) index++;
    }
    bool match(TokenKind type) const {
        return current().type == type;
    }
};

class Parser {
private:
    TokenStream& stream;
    ErrorReporter& reporter;
    int loopDepth;

    Token current() {
        return stream.current();
    }

    Token peek(int offset = 0) {
        return stream.peek(offset);
    }

    void advance() {
        stream.advance();
    }

    void error(const string& msg) {
        int line = current().line;
        reporter.report(line, msg);
    }

    bool match(TokenKind type) {
        return current().type == type;
    }

    bool consume(TokenKind type, const string& errMsg) {
        if (match(type)) {
            advance();
            return true;
        }
        error(errMsg);
        return false;
    }

    void sync() {
        while (!match(TokenKind::Eof) && !match(TokenKind::Semicolon) && !match(TokenKind::RBrace)) {
            advance();
        }
        if (match(TokenKind::Semicolon)) advance();
    }

    void parseCompUnit() {
        while (!match(TokenKind::Eof)) {
            parseFuncDef();
        }
    }

    void parseFuncDef() {
        if (!match(TokenKind::Int) && !match(TokenKind::Void)) {
            error("Expected function return type");
            sync();
            if (match(TokenKind::RBrace)) advance();
            return;
        }
        advance();

        if (!consume(TokenKind::Id, "Expected function name")) {
            sync();
            if (match(TokenKind::RBrace)) advance();
            return;
        }

        consume(TokenKind::LParen, "Lack of '('");

        if (match(TokenKind::Int)) {
            parseParam();
            while (match(TokenKind::Comma)) {
                advance();
                parseParam();
            }
        }

        consume(TokenKind::RParen, "Lack of ')'");
        parseBlock();
    }

    void parseParam() {
        consume(TokenKind::Int, "Expected int");
        consume(TokenKind::Id, "Expected identifier");
    }

    void parseBlock() {
        if (!consume(TokenKind::LBrace, "Lack of '{'")) {
            return;
        }

        while (!match(TokenKind::RBrace) && !match(TokenKind::Eof)) {
            parseStmt();
        }

        consume(TokenKind::RBrace, "Lack of '}'");
    }

    void parseStmt() {
        if (match(TokenKind::Int)) {
            advance();
            consume(TokenKind::Id, "Expected identifier");
            if (match(TokenKind::Assign)) {
                advance();
                parseExpr();
            }
            while (match(TokenKind::Comma)) {
                advance();
                consume(TokenKind::Id, "Expected identifier");
                if (match(TokenKind::Assign)) {
                    advance();
                    parseExpr();
                }
            }
            consume(TokenKind::Semicolon, "Lack of ';'");
        } else if (match(TokenKind::If)) {
            advance();
            consume(TokenKind::LParen, "Lack of '('");
            parseExpr();
            consume(TokenKind::RParen, "Lack of ')'");
            parseStmt();
            if (match(TokenKind::Else)) {
                advance();
                parseStmt();
            }
        } else if (match(TokenKind::While)) {
            advance();
            consume(TokenKind::LParen, "Lack of '('");
            parseExpr();
            consume(TokenKind::RParen, "Lack of ')'");
            loopDepth++;
            parseStmt();
            loopDepth--;
        } else if (match(TokenKind::Break)) {
            advance();
            consume(TokenKind::Semicolon, "Lack of ';'");
        } else if (match(TokenKind::Continue)) {
            advance();
            consume(TokenKind::Semicolon, "Lack of ';'");
        } else if (match(TokenKind::Return)) {
            advance();
            if (!match(TokenKind::Semicolon)) {
                parseExpr();
            }
            consume(TokenKind::Semicolon, "Lack of ';'");
        } else if (match(TokenKind::LBrace)) {
            parseBlock();
        } else if (match(TokenKind::Id)) {
            advance();
            if (match(TokenKind::Assign)) {
                advance();
                parseExpr();
                consume(TokenKind::Semicolon, "Lack of ';'");
            } else if (match(TokenKind::LParen)) {
                advance();
                if (!match(TokenKind::RParen)) {
                    parseExpr();
                    while (match(TokenKind::Comma)) {
                        advance();
                        parseExpr();
                    }
                }
                consume(TokenKind::RParen, "Lack of ')'");
                consume(TokenKind::Semicolon, "Lack of ';'");
            } else {
                consume(TokenKind::Semicolon, "Lack of ';'");
            }
        } else if (match(TokenKind::Semicolon)) {
            advance();
        } else {
            error("Unexpected token");
            advance();
        }
    }

    void parseExpr() {
        parseLOrExpr();
    }

    // LOrExpr → LAndExpr ("||" LAndExpr)*
    void parseLOrExpr() {
        parseLAndExpr();
        while (match(TokenKind::Or)) {
            advance();
            parseLAndExpr();
        }
    }

    // LAndExpr → RelExpr ("&&" RelExpr)*
    void parseLAndExpr() {
        parseRelExpr();
        while (match(TokenKind::And)) {
            advance();
            parseRelExpr();
        }
    }

    // RelExpr → AddExpr (("<" | ">" | ...) AddExpr)*
    void parseRelExpr() {
        parseAddExpr();
        while (match(TokenKind::Lt) || match(TokenKind::Le) || match(TokenKind::Gt) || 
               match(TokenKind::Ge) || match(TokenKind::Eq) || match(TokenKind::Ne)) {
            advance();
            parseAddExpr();
        }
    }

    // AddExpr → MulExpr (("+" | "-") MulExpr)*
    void parseAddExpr() {
        parseMulExpr();
        while (match(TokenKind::Plus) || match(TokenKind::Minus)) {
            advance();
            parseMulExpr();
        }
    }

    // MulExpr → UnaryExpr (("*" | "/" | "%") UnaryExpr)*
    void parseMulExpr() {
        parseUnaryExpr();
        while (match(TokenKind::Star) || match(TokenKind::Div) || match(TokenKind::Mod)) {
            advance();
            parseUnaryExpr();
        }
    }

    void parseUnaryExpr() {
        if (match(TokenKind::Plus) || match(TokenKind::Minus) || match(TokenKind::Not)) {
            advance();
            parseUnaryExpr();
        } else {
            parsePrimaryExpr();
        }
    }

    void parsePrimaryExpr() {
        if (match(TokenKind::Id)) {
            advance();
            if (match(TokenKind::LParen)) {
                advance();
                if (!match(TokenKind::RParen)) {
                    parseExpr();
                    while (match(TokenKind::Comma)) {
                        advance();
                        parseExpr();
                    }
                }
                consume(TokenKind::RParen, "Lack of ')'");
            }
        } else if (match(TokenKind::Number)) {
            advance();
        } else if (match(TokenKind::LParen)) {
            advance();
            parseExpr();
            consume(TokenKind::RParen, "Lack of ')'");
        } else {
            error("Expected expression");
            if (!match(TokenKind::Eof) && !match(TokenKind::Semicolon)) {
                advance();
            }
        }
    }

public:
    Parser(TokenStream& ts, ErrorReporter& rep) : stream(ts), reporter(rep), loopDepth(0) {}

    bool parse() {
        parseCompUnit();
        return !reporter.hasError();
    }
};

int main() {
    string input, line;
    while (getline(cin, line)) {
        input += line + "\n";
    }

    ErrorReporter reporter;
    Lexer lexer(input, reporter);
    vector<Token> tokens;
    
    while (true) {
        Token tok = lexer.nextToken();
        tokens.emplace_back(tok);
        if (tok.type == TokenKind::Eof) break;
    }

    TokenStream stream(tokens);

    Parser parser(stream, reporter);
    bool success = parser.parse();

    if (!reporter.hasError()) {
        cout << "accept" << endl;
    } else {
        cout << "reject" << endl;
        for (const auto& e : reporter.errors()) {
            cout << e.first << " " << e.second << endl;
        }
    }

    return 0;
}