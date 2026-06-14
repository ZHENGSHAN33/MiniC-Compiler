#pragma once

#include "common.hpp"
#include <string>
#include <vector>

namespace minic {

class Lexer {
public:
    explicit Lexer(std::string source);

    TokenList scan();
    const std::vector<CompileError>& errors() const;

private:
    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    TokenList tokens_;
    std::vector<CompileError> errors_;

    bool isAtEnd() const;
    char peek(int offset = 0) const;
    char advance();
    bool match(char expected);
    void add(TokenKind kind, const std::string& lexeme, SourceLocation loc);
    void error(SourceLocation loc, const std::string& message);
    void skipSpaceAndComments();
    void scanIdent(SourceLocation loc);
    void scanNumber(SourceLocation loc);
    void scanSymbol(SourceLocation loc);
};

std::ostream& printTokens(std::ostream& out, const TokenList& tokens);

} // namespace minic