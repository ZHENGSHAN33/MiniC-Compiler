#include "lexer.hpp"
#include "common.hpp"
#include <cctype>
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace minic {

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

TokenList Lexer::scan() {
    while (!isAtEnd()) {
        skipSpaceAndComments();
        if (isAtEnd()) break;
        SourceLocation start{line_, column_};
        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            scanIdent(start);
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            scanNumber(start);
        } else {
            scanSymbol(start);
        }
    }
    tokens_.push_back({TokenKind::END_OF_FILE, "", {line_, column_}});
    return tokens_;
}

const std::vector<CompileError>& Lexer::errors() const {
    return errors_;
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

char Lexer::peek(int offset) const {
    size_t index = pos_ + static_cast<size_t>(offset);
    return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    advance();
    return true;
}

void Lexer::add(TokenKind kind, const std::string& lexeme, SourceLocation loc) {
    tokens_.push_back({kind, lexeme, loc});
}

void Lexer::error(SourceLocation loc, const std::string& message) {
    errors_.push_back({"LexicalError", loc, message});
}

void Lexer::skipSpaceAndComments() {
    bool moved = true;
    while (moved && !isAtEnd()) {
        moved = false;
        while (std::isspace(static_cast<unsigned char>(peek()))) {
            advance();
            moved = true;
        }
        if (peek() == '/' && peek(1) == '/') {
            while (!isAtEnd() && peek() != '\n') advance();
            moved = true;
        } else if (peek() == '/' && peek(1) == '*') {
            SourceLocation loc{line_, column_};
            advance();
            advance();
            while (!isAtEnd() && !(peek() == '*' && peek(1) == '/')) advance();
            if (isAtEnd()) {
                error(loc, "unclosed block comment");
                return;
            }
            advance();
            advance();
            moved = true;
        }
    }
}

void Lexer::scanIdent(SourceLocation loc) {
    std::string text;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        text.push_back(advance());
    }
    static const std::unordered_map<std::string, TokenKind> keywords = {
        {"int", TokenKind::KW_INT}, {"bool", TokenKind::KW_BOOL},
        {"if", TokenKind::KW_IF}, {"else", TokenKind::KW_ELSE},
        {"while", TokenKind::KW_WHILE}, {"return", TokenKind::KW_RETURN},
        {"break", TokenKind::KW_BREAK}, {"continue", TokenKind::KW_CONTINUE},
        {"read", TokenKind::KW_READ}, {"write", TokenKind::KW_WRITE},
        {"true", TokenKind::KW_TRUE}, {"false", TokenKind::KW_FALSE},
    };
    auto it = keywords.find(text);
    add(it == keywords.end() ? TokenKind::IDENT : it->second, text, loc);
}

void Lexer::scanNumber(SourceLocation loc) {
    std::string text;
    while (std::isdigit(static_cast<unsigned char>(peek()))) text.push_back(advance());
    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_') {
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') text.push_back(advance());
        error(loc, "illegal number format '" + text + "'");
        return;
    }
    add(TokenKind::INT_LITERAL, text, loc);
}

void Lexer::scanSymbol(SourceLocation loc) {
    char c = advance();
    switch (c) {
        case '+': add(TokenKind::PLUS, "+", loc); break;
        case '-': add(TokenKind::MINUS, "-", loc); break;
        case '*': add(TokenKind::STAR, "*", loc); break;
        case '/': add(TokenKind::SLASH, "/", loc); break;
        case '%': add(TokenKind::PERCENT, "%", loc); break;
        case ';': add(TokenKind::SEMICOLON, ";", loc); break;
        case ',': add(TokenKind::COMMA, ",", loc); break;
        case '(': add(TokenKind::LPAREN, "(", loc); break;
        case ')': add(TokenKind::RPAREN, ")", loc); break;
        case '{': add(TokenKind::LBRACE, "{", loc); break;
        case '}': add(TokenKind::RBRACE, "}", loc); break;
        case '=': {
            bool two = match('=');
            add(two ? TokenKind::EQ : TokenKind::ASSIGN, two ? "==" : "=", loc);
            break;
        }
        case '!': {
            bool two = match('=');
            add(two ? TokenKind::NE : TokenKind::NOT, two ? "!=" : "!", loc);
            break;
        }
        case '<': {
            bool two = match('=');
            add(two ? TokenKind::LE : TokenKind::LT, two ? "<=" : "<", loc);
            break;
        }
        case '>': {
            bool two = match('=');
            add(two ? TokenKind::GE : TokenKind::GT, two ? ">=" : ">", loc);
            break;
        }
        case '&':
            if (match('&')) add(TokenKind::AND, "&&", loc);
            else error(loc, "expected '&' after '&'");
            break;
        case '|':
            if (match('|')) add(TokenKind::OR, "||", loc);
            else error(loc, "expected '|' after '|'");
            break;
        default:
            error(loc, std::string("unexpected character '") + c + "'");
            break;
    }
}

namespace {

std::string tokenName(TokenKind kind) {
    switch (kind) {
        case TokenKind::KW_INT: return "KW_INT";
        case TokenKind::KW_BOOL: return "KW_BOOL";
        case TokenKind::KW_IF: return "KW_IF";
        case TokenKind::KW_ELSE: return "KW_ELSE";
        case TokenKind::KW_WHILE: return "KW_WHILE";
        case TokenKind::KW_RETURN: return "KW_RETURN";
        case TokenKind::KW_BREAK: return "KW_BREAK";
        case TokenKind::KW_CONTINUE: return "KW_CONTINUE";
        case TokenKind::KW_READ: return "KW_READ";
        case TokenKind::KW_WRITE: return "KW_WRITE";
        case TokenKind::KW_TRUE: return "KW_TRUE";
        case TokenKind::KW_FALSE: return "KW_FALSE";
        case TokenKind::IDENT: return "IDENT";
        case TokenKind::INT_LITERAL: return "INT_LITERAL";
        case TokenKind::PLUS: return "PLUS";
        case TokenKind::MINUS: return "MINUS";
        case TokenKind::STAR: return "STAR";
        case TokenKind::SLASH: return "SLASH";
        case TokenKind::PERCENT: return "PERCENT";
        case TokenKind::ASSIGN: return "ASSIGN";
        case TokenKind::EQ: return "EQ";
        case TokenKind::NE: return "NE";
        case TokenKind::LT: return "LT";
        case TokenKind::LE: return "LE";
        case TokenKind::GT: return "GT";
        case TokenKind::GE: return "GE";
        case TokenKind::AND: return "AND";
        case TokenKind::OR: return "OR";
        case TokenKind::NOT: return "NOT";
        case TokenKind::SEMICOLON: return "SEMICOLON";
        case TokenKind::COMMA: return "COMMA";
        case TokenKind::LPAREN: return "LPAREN";
        case TokenKind::RPAREN: return "RPAREN";
        case TokenKind::LBRACE: return "LBRACE";
        case TokenKind::RBRACE: return "RBRACE";
        case TokenKind::END_OF_FILE: return "EOF";
    }
    return "UNKNOWN";
}

} // namespace

std::ostream& printTokens(std::ostream& out, const TokenList& tokens) {
    bool first = true;
    for (const auto& t : tokens) {
        if (t.kind == TokenKind::END_OF_FILE) continue;
        if (!first) out << " ";
        first = false;
        std::string name = tokenName(t.kind);
        if (t.kind == TokenKind::IDENT || t.kind == TokenKind::INT_LITERAL) {
            out << name << "(" << t.lexeme << ")";
        } else {
            out << name;
        }
    }
    out << "\n";
    return out;
}

} // namespace minic