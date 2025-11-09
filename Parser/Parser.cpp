#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <cctype>

enum class TokenType {
    INT, VOID, IF, ELSE, WHILE, BREAK, CONTINUE, RETURN,
    ID, NUMBER,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NE, LT, LE, GT, GE,
    AND, OR, NOT,
    LPAREN, RPAREN, LBRACE, RBRACE,
    COMMA, SEMICOLON,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

class Lexer {
public:
    Lexer(const std::string& input, std::set<std::string>& errors, bool& hasError)
        : input_(input), pos_(0), line_(1), errors_(errors), hasError_(hasError) {
        keywords_ = {
            {"int", TokenType::INT},       {"void", TokenType::VOID},
            {"if", TokenType::IF},         {"else", TokenType::ELSE},
            {"while", TokenType::WHILE},   {"break", TokenType::BREAK},
            {"continue", TokenType::CONTINUE}, {"return", TokenType::RETURN}
        };
    }

    Token getNextToken() {
        skipWhitespaceAndComments();

        if (pos_ >= input_.length()) {
            return {TokenType::END_OF_FILE, "", line_};
        }

        char current = input_[pos_];

        if (std::isalpha(current) || current == '_') {
            return identifier();
        }

        if (std::isdigit(current)) {
            return number();
        }

        switch (current) {
            case '+': return makeToken(TokenType::PLUS, "+");
            case '-': return makeToken(TokenType::MINUS, "-");
            case '*': return makeToken(TokenType::STAR, "*");
            case '/': return makeToken(TokenType::SLASH, "/");
            case '%': return makeToken(TokenType::PERCENT, "%");
            case '(': return makeToken(TokenType::LPAREN, "(");
            case ')': return makeToken(TokenType::RPAREN, ")");
            case '{': return makeToken(TokenType::LBRACE, "{");
            case '}': return makeToken(TokenType::RBRACE, "}");
            case ',': return makeToken(TokenType::COMMA, ",");
            case ';': return makeToken(TokenType::SEMICOLON, ";");

            case '=':
                return (peek() == '=') ? (advance(), makeToken(TokenType::EQ, "==")) : makeToken(TokenType::ASSIGN, "=");
            case '!':
                return (peek() == '=') ? (advance(), makeToken(TokenType::NE, "!=")) : makeToken(TokenType::NOT, "!");
            case '<':
                return (peek() == '=') ? (advance(), makeToken(TokenType::LE, "<=")) : makeToken(TokenType::LT, "<");
            case '>':
                return (peek() == '=') ? (advance(), makeToken(TokenType::GE, ">=")) : makeToken(TokenType::GT, ">");
            case '&':
                return (peek() == '&') ? (advance(), makeToken(TokenType::AND, "&&")) : makeToken(TokenType::UNKNOWN, "&");
            case '|':
                return (peek() == '|') ? (advance(), makeToken(TokenType::OR, "||")) : makeToken(TokenType::UNKNOWN, "|");
        
            default:
                lexerError("未知字符");
                advance();
                return makeToken(TokenType::UNKNOWN, std::string(1, current));
        }
    }

private:
    std::string input_;
    size_t pos_;
    int line_;
    std::map<std::string, TokenType> keywords_;
    std::set<std::string>& errors_;
    bool& hasError_;

    void advance() {
        if (pos_ < input_.length()) {
            pos_++;
        }
    }

    char peek() {
        if (pos_ + 1 >= input_.length()) return '\0';
        return input_[pos_ + 1];
    }

    char peekNext() {
        if (pos_ + 2 >= input_.length()) return '\0';
        return input_[pos_ + 2];
    }

    Token makeToken(TokenType type, const std::string& lexeme) {
        advance();
        return {type, lexeme, line_};
    }

    void skipWhitespaceAndComments() {
        while (pos_ < input_.length()) {
            char current = input_[pos_];
            if (std::isspace(current)) {
                if (current == '\n') {
                    line_++;
                }
                advance();
            } else if (current == '/' && peek() == '/') {
                while (pos_ < input_.length() && input_[pos_] != '\n') {
                    advance();
                }
            } else if (current == '/' && peek() == '*') {
                int startLine = line_;
                advance();
                advance();
                while (pos_ < input_.length()) {
                    if (input_[pos_] == '*' && peek() == '/') {
                        advance();
                        advance();
                        break;
                    }
                    if (input_[pos_] == '\n') {
                        line_++;
                    }
                    advance();
                }
                if (pos_ >= input_.length()) {
                    lexerError("未终止的多行注释", startLine);
                }
            } else {
                break;
            }
        }
    }

    Token identifier() {
        std::string lexeme;
        int startLine = line_;
        while (pos_ < input_.length() && (std::isalnum(input_[pos_]) || input_[pos_] == '_')) {
            lexeme += input_[pos_];
            advance();
        }
        auto it = keywords_.find(lexeme);
        if (it != keywords_.end()) {
            return {it->second, lexeme, startLine};
        }
        return {TokenType::ID, lexeme, startLine};
    }

    Token number() {
        std::string lexeme;
        int startLine = line_;
        if (input_[pos_] == '0') {
            lexeme += '0';
            advance();
        } else {
            while (pos_ < input_.length() && std::isdigit(input_[pos_])) {
                lexeme += input_[pos_];
                advance();
            }
        }
        return {TokenType::NUMBER, lexeme, startLine};
    }

    void lexerError(const std::string& message, int line) {
        hasError_ = true;
        errors_.insert(std::to_string(line) + " " + message);
    }
    void lexerError(const std::string& message) {
        lexerError(message, line_);
    }
};

class Parser {
public:
    Parser(Lexer& lexer, std::set<std::string>& errors, bool& hasError)
        : lexer_(lexer), errors_(errors), hasError_(hasError) {
        advance();
        advance();
    }

    void parse() {
        parseCompUnit();
        if (currentToken_.type != TokenType::END_OF_FILE && !hasError_) {
            reportError("在程序结束后出现意外的 token");
        }
    }

private:
    Lexer& lexer_;
    Token currentToken_;
    Token peekToken_;
    std::set<std::string>& errors_;
    bool& hasError_;

    void advance() {
        currentToken_ = peekToken_;
        peekToken_ = lexer_.getNextToken();
    }

    bool eat(TokenType expected) {
        if (currentToken_.type == expected) {
            advance();
            return true;
        } else {
            reportError();
            return false;
        }
    }

    void reportError(const std::string& message = "") {
        hasError_ = true;
        std::string errorLine = std::to_string(currentToken_.line);
        if (!message.empty()) {
            errorLine += " " + message;
        }
        errors_.insert(errorLine);
    }

    void parseCompUnit() {
        if (currentToken_.type == TokenType::END_OF_FILE) {
             reportError("程序为空，至少需要一个函数定义");
             return;
        }
        
        while (currentToken_.type != TokenType::END_OF_FILE) {
            parseFuncDef();
        }
    }

    void parseFuncDef() {
        if (currentToken_.type != TokenType::INT && currentToken_.type != TokenType::VOID) {
            reportError("期望函数返回类型 (int 或 void)");
            while(currentToken_.type != TokenType::INT && 
                  currentToken_.type != TokenType::VOID && 
                  currentToken_.type != TokenType::END_OF_FILE) {
                advance();
            }
            if (currentToken_.type == TokenType::END_OF_FILE) return;
        }
        advance();

        eat(TokenType::ID);
        eat(TokenType::LPAREN);

        if (currentToken_.type == TokenType::INT) {
            parseParams();
        }

        eat(TokenType::RPAREN);
        parseBlock();
    }

    void parseParams() {
        parseParam();
        while (currentToken_.type == TokenType::COMMA) {
            advance();
            parseParam();
        }
    }

    void parseParam() {
        eat(TokenType::INT);
        eat(TokenType::ID);
    }

    void parseBlock() {
        if (!eat(TokenType::LBRACE)) {
            return;
        }

        while (currentToken_.type != TokenType::RBRACE && currentToken_.type != TokenType::END_OF_FILE) {
            parseStmt();
        }

        eat(TokenType::RBRACE);
    }

    void parseStmt() {
        switch (currentToken_.type) {
            case TokenType::LBRACE:
                parseBlock();
                break;
            case TokenType::SEMICOLON:
                advance();
                break;
            case TokenType::INT:
                advance();
                eat(TokenType::ID);
                eat(TokenType::ASSIGN);
                parseExpr();
                eat(TokenType::SEMICOLON);
                break;
            case TokenType::IF:
                advance();
                eat(TokenType::LPAREN);
                parseExpr();
                eat(TokenType::RPAREN);
                parseStmt();
                if (currentToken_.type == TokenType::ELSE) {
                    advance();
                    parseStmt();
                }
                break;
            case TokenType::WHILE:
                advance();
                eat(TokenType::LPAREN);
                parseExpr();
                eat(TokenType::RPAREN);
                parseStmt();
                break;
            case TokenType::BREAK:
                advance();
                eat(TokenType::SEMICOLON);
                break;
            case TokenType::CONTINUE:
                advance();
                eat(TokenType::SEMICOLON);
                break;
            case TokenType::RETURN:
                advance();
                parseExpr();
                eat(TokenType::SEMICOLON);
                break;

            case TokenType::ID:
                if (peekToken_.type == TokenType::ASSIGN) {
                    advance(); 
                    advance(); 
                    parseExpr();
                    eat(TokenType::SEMICOLON);
                } else {
                    parseExpr();
                    eat(TokenType::SEMICOLON);
                }
                break;
            
            default:
                if (currentToken_.type == TokenType::LPAREN ||
                    currentToken_.type == TokenType::NUMBER ||
                    currentToken_.type == TokenType::PLUS ||
                    currentToken_.type == TokenType::MINUS ||
                    currentToken_.type == TokenType::NOT) 
                {
                    parseExpr();
                    eat(TokenType::SEMICOLON);
                } else {
                    reportError("期望一个语句");
                    advance();
                }
                break;
        }
    }

    void parseExpr() {
        parseLOrExpr();
    }

    void parseLOrExpr() {
        parseLAndExpr();
        while (currentToken_.type == TokenType::OR) {
            advance();
            parseLAndExpr();
        }
    }

    void parseLAndExpr() {
        parseRelExpr();
        while (currentToken_.type == TokenType::AND) {
            advance();
            parseRelExpr();
        }
    }

    void parseRelExpr() {
        parseAddExpr();
        while (currentToken_.type == TokenType::LT || currentToken_.type == TokenType::GT ||
               currentToken_.type == TokenType::LE || currentToken_.type == TokenType::GE ||
               currentToken_.type == TokenType::EQ || currentToken_.type == TokenType::NE) {
            advance();
            parseAddExpr();
        }
    }

    void parseAddExpr() {
        parseMulExpr();
        while (currentToken_.type == TokenType::PLUS || currentToken_.type == TokenType::MINUS) {
            advance();
            parseMulExpr();
        }
    }

    void parseMulExpr() {
        parseUnaryExpr();
        while (currentToken_.type == TokenType::STAR || currentToken_.type == TokenType::SLASH || currentToken_.type == TokenType::PERCENT) {
            advance();
            parseUnaryExpr();
        }
    }

    void parseUnaryExpr() {
        if (currentToken_.type == TokenType::PLUS || currentToken_.type == TokenType::MINUS || currentToken_.type == TokenType::NOT) {
            advance();
            parseUnaryExpr();
        } else {
            parsePrimaryExpr();
        }
    }

    void parsePrimaryExpr() {
        switch (currentToken_.type) {
            case TokenType::ID:
                advance();
                if (currentToken_.type == TokenType::LPAREN) {
                    advance(); 
                    
                    if (currentToken_.type != TokenType::RPAREN) {
                        parseArgs();
                    }
                    
                    eat(TokenType::RPAREN);
                }
                break;
            case TokenType::NUMBER:
                advance();
                break;
            case TokenType::LPAREN:
                advance(); 
                parseExpr();
                eat(TokenType::RPAREN);
                break;
            default:
                reportError("期望 标识符(ID)、数字、或 (表达式)");
                break;
        }
    }

    void parseArgs() {
        parseExpr();
        while (currentToken_.type == TokenType::COMMA) {
            advance(); 
            parseExpr();
        }
    }
};

int main() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    std::string input = ss.str();

    std::set<std::string> errors; 
    bool hasError = false;

    Lexer lexer(input, errors, hasError);
    Parser parser(lexer, errors, hasError);
    
    try {
        parser.parse();
    } catch (const std::exception& e) {
        hasError = true;
        errors.insert("0 " + std::string(e.what()));
    }

    if (hasError) {
        std::cout << "reject" << std::endl;
        for (const auto& errLine : errors) {
            std::cout << errLine.substr(0, errLine.find(' ')) << std::endl;
        }
    } else {
        std::cout << "accept" << std::endl;
    }

    return 0;
}