#include "common.hpp"
#include "../include/ir.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>

using namespace minic;

namespace
{

    std::string readFile(const std::string &path)
    {
        std::ifstream in(path);
        if (!in)
            throw std::runtime_error("cannot open input file: " + path);
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    std::string tokenName(TokenKind kind)
    {
        switch (kind)
        {
        case TokenKind::KW_INT:
            return "KW_INT";
        case TokenKind::KW_BOOL:
            return "KW_BOOL";
        case TokenKind::KW_IF:
            return "KW_IF";
        case TokenKind::KW_ELSE:
            return "KW_ELSE";
        case TokenKind::KW_WHILE:
            return "KW_WHILE";
        case TokenKind::KW_RETURN:
            return "KW_RETURN";
        case TokenKind::KW_BREAK:
            return "KW_BREAK";
        case TokenKind::KW_CONTINUE:
            return "KW_CONTINUE";
        case TokenKind::KW_READ:
            return "KW_READ";
        case TokenKind::KW_WRITE:
            return "KW_WRITE";
        case TokenKind::KW_TRUE:
            return "KW_TRUE";
        case TokenKind::KW_FALSE:
            return "KW_FALSE";
        case TokenKind::IDENT:
            return "IDENT";
        case TokenKind::INT_LITERAL:
            return "INT_LITERAL";
        case TokenKind::PLUS:
            return "PLUS";
        case TokenKind::MINUS:
            return "MINUS";
        case TokenKind::STAR:
            return "STAR";
        case TokenKind::SLASH:
            return "SLASH";
        case TokenKind::PERCENT:
            return "PERCENT";
        case TokenKind::ASSIGN:
            return "ASSIGN";
        case TokenKind::EQ:
            return "EQ";
        case TokenKind::NE:
            return "NE";
        case TokenKind::LT:
            return "LT";
        case TokenKind::LE:
            return "LE";
        case TokenKind::GT:
            return "GT";
        case TokenKind::GE:
            return "GE";
        case TokenKind::AND:
            return "AND";
        case TokenKind::OR:
            return "OR";
        case TokenKind::NOT:
            return "NOT";
        case TokenKind::SEMICOLON:
            return "SEMICOLON";
        case TokenKind::COMMA:
            return "COMMA";
        case TokenKind::LPAREN:
            return "LPAREN";
        case TokenKind::RPAREN:
            return "RPAREN";
        case TokenKind::LBRACE:
            return "LBRACE";
        case TokenKind::RBRACE:
            return "RBRACE";
        case TokenKind::END_OF_FILE:
            return "EOF";
        }
        return "UNKNOWN";
    }

    std::string typeName(TypeKind type)
    {
        switch (type)
        {
        case TypeKind::Int:
            return "int";
        case TypeKind::Bool:
            return "bool";
        case TypeKind::Void:
            return "void";
        case TypeKind::Error:
            return "error";
        }
        return "error";
    }

    std::string astName(ASTKind kind)
    {
        switch (kind)
        {
        case ASTKind::Program:
            return "Program";
        case ASTKind::Function:
            return "Function";
        case ASTKind::Block:
            return "Block";
        case ASTKind::VarDecl:
            return "VarDecl";
        case ASTKind::IfStmt:
            return "IfStmt";
        case ASTKind::WhileStmt:
            return "WhileStmt";
        case ASTKind::BreakStmt:
            return "BreakStmt";
        case ASTKind::ContinueStmt:
            return "ContinueStmt";
        case ASTKind::ReturnStmt:
            return "ReturnStmt";
        case ASTKind::ReadStmt:
            return "ReadStmt";
        case ASTKind::WriteStmt:
            return "WriteStmt";
        case ASTKind::AssignStmt:
            return "AssignStmt";
        case ASTKind::BinaryExpr:
            return "BinaryExpr";
        case ASTKind::UnaryExpr:
            return "UnaryExpr";
        case ASTKind::IntLiteral:
            return "IntLiteral";
        case ASTKind::BoolLiteral:
            return "BoolLiteral";
        case ASTKind::Identifier:
            return "Identifier";
        }
        return "Node";
    }

    bool isIntegerText(const std::string &s)
    {
        if (s.empty())
            return false;
        size_t i = (s[0] == '-') ? 1 : 0;
        if (i == s.size())
            return false;
        for (; i < s.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(s[i])))
                return false;
        return true;
    }

    class Lexer
    {
    public:
        explicit Lexer(std::string source) : source_(std::move(source)) {}

        TokenList scan()
        {
            while (!isAtEnd())
            {
                skipSpaceAndComments();
                if (isAtEnd())
                    break;
                SourceLocation start{line_, column_};
                char c = peek();
                if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
                {
                    scanIdent(start);
                }
                else if (std::isdigit(static_cast<unsigned char>(c)))
                {
                    scanNumber(start);
                }
                else
                {
                    scanSymbol(start);
                }
            }
            tokens_.push_back({TokenKind::END_OF_FILE, "", {line_, column_}});
            return tokens_;
        }

        const std::vector<CompileError> &errors() const { return errors_; }

    private:
        std::string source_;
        size_t pos_ = 0;
        int line_ = 1;
        int column_ = 1;
        TokenList tokens_;
        std::vector<CompileError> errors_;

        bool isAtEnd() const { return pos_ >= source_.size(); }
        char peek(int offset = 0) const
        {
            size_t index = pos_ + static_cast<size_t>(offset);
            return index < source_.size() ? source_[index] : '\0';
        }

        char advance()
        {
            char c = source_[pos_++];
            if (c == '\n')
            {
                ++line_;
                column_ = 1;
            }
            else
            {
                ++column_;
            }
            return c;
        }

        bool match(char expected)
        {
            if (peek() != expected)
                return false;
            advance();
            return true;
        }

        void add(TokenKind kind, const std::string &lexeme, SourceLocation loc)
        {
            tokens_.push_back({kind, lexeme, loc});
        }

        void error(SourceLocation loc, const std::string &message)
        {
            errors_.push_back({"LexicalError", loc, message});
        }

        void skipSpaceAndComments()
        {
            bool moved = true;
            while (moved && !isAtEnd())
            {
                moved = false;
                while (std::isspace(static_cast<unsigned char>(peek())))
                {
                    advance();
                    moved = true;
                }
                if (peek() == '/' && peek(1) == '/')
                {
                    while (!isAtEnd() && peek() != '\n')
                        advance();
                    moved = true;
                }
                else if (peek() == '/' && peek(1) == '*')
                {
                    SourceLocation loc{line_, column_};
                    advance();
                    advance();
                    while (!isAtEnd() && !(peek() == '*' && peek(1) == '/'))
                        advance();
                    if (isAtEnd())
                    {
                        error(loc, "unclosed block comment");
                        return;
                    }
                    advance();
                    advance();
                    moved = true;
                }
            }
        }

        void scanIdent(SourceLocation loc)
        {
            std::string text;
            while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
            {
                text.push_back(advance());
            }
            static const std::unordered_map<std::string, TokenKind> keywords = {
                {"int", TokenKind::KW_INT},
                {"bool", TokenKind::KW_BOOL},
                {"if", TokenKind::KW_IF},
                {"else", TokenKind::KW_ELSE},
                {"while", TokenKind::KW_WHILE},
                {"return", TokenKind::KW_RETURN},
                {"break", TokenKind::KW_BREAK},
                {"continue", TokenKind::KW_CONTINUE},
                {"read", TokenKind::KW_READ},
                {"write", TokenKind::KW_WRITE},
                {"true", TokenKind::KW_TRUE},
                {"false", TokenKind::KW_FALSE},
            };
            auto it = keywords.find(text);
            add(it == keywords.end() ? TokenKind::IDENT : it->second, text, loc);
        }

        void scanNumber(SourceLocation loc)
        {
            std::string text;
            while (std::isdigit(static_cast<unsigned char>(peek())))
                text.push_back(advance());
            if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_')
            {
                while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
                    text.push_back(advance());
                error(loc, "illegal number format '" + text + "'");
                return;
            }
            add(TokenKind::INT_LITERAL, text, loc);
        }

        void scanSymbol(SourceLocation loc)
        {
            char c = advance();
            switch (c)
            {
            case '+':
                add(TokenKind::PLUS, "+", loc);
                break;
            case '-':
                add(TokenKind::MINUS, "-", loc);
                break;
            case '*':
                add(TokenKind::STAR, "*", loc);
                break;
            case '/':
                add(TokenKind::SLASH, "/", loc);
                break;
            case '%':
                add(TokenKind::PERCENT, "%", loc);
                break;
            case ';':
                add(TokenKind::SEMICOLON, ";", loc);
                break;
            case ',':
                add(TokenKind::COMMA, ",", loc);
                break;
            case '(':
                add(TokenKind::LPAREN, "(", loc);
                break;
            case ')':
                add(TokenKind::RPAREN, ")", loc);
                break;
            case '{':
                add(TokenKind::LBRACE, "{", loc);
                break;
            case '}':
                add(TokenKind::RBRACE, "}", loc);
                break;
            case '=':
            {
                bool two = match('=');
                add(two ? TokenKind::EQ : TokenKind::ASSIGN, two ? "==" : "=", loc);
                break;
            }
            case '!':
            {
                bool two = match('=');
                add(two ? TokenKind::NE : TokenKind::NOT, two ? "!=" : "!", loc);
                break;
            }
            case '<':
            {
                bool two = match('=');
                add(two ? TokenKind::LE : TokenKind::LT, two ? "<=" : "<", loc);
                break;
            }
            case '>':
            {
                bool two = match('=');
                add(two ? TokenKind::GE : TokenKind::GT, two ? ">=" : ">", loc);
                break;
            }
            case '&':
                if (match('&'))
                    add(TokenKind::AND, "&&", loc);
                else
                    error(loc, "expected '&' after '&'");
                break;
            case '|':
                if (match('|'))
                    add(TokenKind::OR, "||", loc);
                else
                    error(loc, "expected '|' after '|'");
                break;
            default:
                error(loc, std::string("unexpected character '") + c + "'");
                break;
            }
        }
    };

    std::ostream &printTokens(std::ostream &out, const TokenList &tokens)
    {
        for (const auto &t : tokens)
        {
            if (t.kind == TokenKind::END_OF_FILE)
                continue;
            out << std::setw(3) << t.loc.line << ":" << std::left << std::setw(4) << t.loc.column
                << " " << std::setw(14) << tokenName(t.kind) << " " << t.lexeme << "\n";
        }
        return out;
    }

    std::unique_ptr<ASTNode> makeNode(ASTKind kind, const std::string &text, SourceLocation loc)
    {
        return std::make_unique<ASTNode>(kind, text, loc);
    }

    class Parser
    {
    public:
        explicit Parser(const TokenList &tokens) : tokens_(tokens) {}

        std::unique_ptr<ASTNode> parseProgram()
        {
            auto program = makeNode(ASTKind::Program, "", current().loc);
            while (!check(TokenKind::END_OF_FILE))
            {
                program->children.push_back(parseFunction());
                if (!errors_.empty())
                    synchronizeTop();
            }
            return program;
        }

        const std::vector<CompileError> &errors() const { return errors_; }

    private:
        const TokenList &tokens_;
        size_t pos_ = 0;
        std::vector<CompileError> errors_;

        const Token &current() const { return tokens_[std::min(pos_, tokens_.size() - 1)]; }
        const Token &previous() const { return tokens_[pos_ - 1]; }
        bool check(TokenKind kind) const { return current().kind == kind; }
        bool match(TokenKind kind)
        {
            if (!check(kind))
                return false;
            ++pos_;
            return true;
        }

        Token consume(TokenKind kind, const std::string &message)
        {
            if (check(kind))
                return tokens_[pos_++];
            error(current().loc, message);
            return {kind, "", current().loc};
        }

        void error(SourceLocation loc, const std::string &message)
        {
            errors_.push_back({"SyntaxError", loc, message});
        }

        void synchronizeStmt()
        {
            while (!check(TokenKind::END_OF_FILE) && !check(TokenKind::RBRACE))
            {
                if (match(TokenKind::SEMICOLON))
                    return;
                if (check(TokenKind::KW_INT) || check(TokenKind::KW_BOOL) || check(TokenKind::KW_IF) ||
                    check(TokenKind::KW_WHILE) || check(TokenKind::KW_RETURN))
                    return;
                ++pos_;
            }
        }

        void synchronizeTop()
        {
            while (!check(TokenKind::END_OF_FILE) && !check(TokenKind::KW_INT))
                ++pos_;
        }

        TypeKind parseType()
        {
            if (match(TokenKind::KW_INT))
                return TypeKind::Int;
            if (match(TokenKind::KW_BOOL))
                return TypeKind::Bool;
            error(current().loc, "expected type name");
            return TypeKind::Error;
        }

        std::unique_ptr<ASTNode> parseFunction()
        {
            SourceLocation loc = current().loc;
            TypeKind ret = parseType();
            Token name = consume(TokenKind::IDENT, "expected function name");
            consume(TokenKind::LPAREN, "expected '(' after function name");
            consume(TokenKind::RPAREN, "expected ')' after function parameters");
            auto fn = makeNode(ASTKind::Function, name.lexeme, loc);
            fn->type = ret;
            fn->children.push_back(parseBlock());
            return fn;
        }

        std::unique_ptr<ASTNode> parseBlock()
        {
            SourceLocation loc = current().loc;
            consume(TokenKind::LBRACE, "expected '{' before block");
            auto block = makeNode(ASTKind::Block, "", loc);
            while (!check(TokenKind::RBRACE) && !check(TokenKind::END_OF_FILE))
            {
                block->children.push_back(parseStmt());
            }
            consume(TokenKind::RBRACE, "expected '}' after block");
            return block;
        }

        std::unique_ptr<ASTNode> parseStmt()
        {
            try
            {
                if (check(TokenKind::KW_INT) || check(TokenKind::KW_BOOL))
                    return parseVarDecl();
                if (check(TokenKind::LBRACE))
                    return parseBlock();
                if (match(TokenKind::KW_IF))
                    return parseIf(previous().loc);
                if (match(TokenKind::KW_WHILE))
                    return parseWhile(previous().loc);
                if (match(TokenKind::KW_BREAK))
                {
                    auto node = makeNode(ASTKind::BreakStmt, "", previous().loc);
                    consume(TokenKind::SEMICOLON, "expected ';' after break");
                    return node;
                }
                if (match(TokenKind::KW_CONTINUE))
                {
                    auto node = makeNode(ASTKind::ContinueStmt, "", previous().loc);
                    consume(TokenKind::SEMICOLON, "expected ';' after continue");
                    return node;
                }
                if (match(TokenKind::KW_RETURN))
                    return parseReturn(previous().loc);
                if (match(TokenKind::KW_READ))
                    return parseRead(previous().loc);
                if (match(TokenKind::KW_WRITE))
                    return parseWrite(previous().loc);
                if (check(TokenKind::IDENT))
                    return parseAssign();
                error(current().loc, "expected statement");
                ++pos_;
                return makeNode(ASTKind::Block, "", current().loc);
            }
            catch (...)
            {
                synchronizeStmt();
                return makeNode(ASTKind::Block, "", current().loc);
            }
        }

        std::unique_ptr<ASTNode> parseVarDecl()
        {
            SourceLocation loc = current().loc;
            TypeKind type = parseType();
            Token name = consume(TokenKind::IDENT, "expected variable name");
            consume(TokenKind::SEMICOLON, "expected ';' after variable declaration");
            auto node = makeNode(ASTKind::VarDecl, name.lexeme, loc);
            node->type = type;
            return node;
        }

        std::unique_ptr<ASTNode> parseIf(SourceLocation loc)
        {
            auto node = makeNode(ASTKind::IfStmt, "", loc);
            consume(TokenKind::LPAREN, "expected '(' after if");
            node->children.push_back(parseExpr());
            consume(TokenKind::RPAREN, "expected ')' after if condition");
            node->children.push_back(parseStmt());
            if (match(TokenKind::KW_ELSE))
                node->children.push_back(parseStmt());
            return node;
        }

        std::unique_ptr<ASTNode> parseWhile(SourceLocation loc)
        {
            auto node = makeNode(ASTKind::WhileStmt, "", loc);
            consume(TokenKind::LPAREN, "expected '(' after while");
            node->children.push_back(parseExpr());
            consume(TokenKind::RPAREN, "expected ')' after while condition");
            node->children.push_back(parseStmt());
            return node;
        }

        std::unique_ptr<ASTNode> parseReturn(SourceLocation loc)
        {
            auto node = makeNode(ASTKind::ReturnStmt, "", loc);
            node->children.push_back(parseExpr());
            consume(TokenKind::SEMICOLON, "expected ';' after return value");
            return node;
        }

        std::unique_ptr<ASTNode> parseRead(SourceLocation loc)
        {
            auto node = makeNode(ASTKind::ReadStmt, "", loc);
            consume(TokenKind::LPAREN, "expected '(' after read");
            Token name = consume(TokenKind::IDENT, "read expects a variable name");
            node->children.push_back(makeNode(ASTKind::Identifier, name.lexeme, name.loc));
            consume(TokenKind::RPAREN, "expected ')' after read argument");
            consume(TokenKind::SEMICOLON, "expected ';' after read statement");
            return node;
        }

        std::unique_ptr<ASTNode> parseWrite(SourceLocation loc)
        {
            auto node = makeNode(ASTKind::WriteStmt, "", loc);
            consume(TokenKind::LPAREN, "expected '(' after write");
            node->children.push_back(parseExpr());
            consume(TokenKind::RPAREN, "expected ')' after write argument");
            consume(TokenKind::SEMICOLON, "expected ';' after write statement");
            return node;
        }

        std::unique_ptr<ASTNode> parseAssign()
        {
            Token name = consume(TokenKind::IDENT, "expected variable name");
            auto node = makeNode(ASTKind::AssignStmt, name.lexeme, name.loc);
            consume(TokenKind::ASSIGN, "expected '=' in assignment");
            node->children.push_back(parseExpr());
            consume(TokenKind::SEMICOLON, "expected ';' after assignment");
            return node;
        }

        std::unique_ptr<ASTNode> parseExpr() { return parseOr(); }

        std::unique_ptr<ASTNode> parseOr()
        {
            auto left = parseAnd();
            while (match(TokenKind::OR))
                left = binary("||", std::move(left), parseAnd(), previous().loc);
            return left;
        }

        std::unique_ptr<ASTNode> parseAnd()
        {
            auto left = parseEquality();
            while (match(TokenKind::AND))
                left = binary("&&", std::move(left), parseEquality(), previous().loc);
            return left;
        }

        std::unique_ptr<ASTNode> parseEquality()
        {
            auto left = parseRelation();
            while (match(TokenKind::EQ) || match(TokenKind::NE))
            {
                std::string op = previous().lexeme;
                left = binary(op, std::move(left), parseRelation(), previous().loc);
            }
            return left;
        }

        std::unique_ptr<ASTNode> parseRelation()
        {
            auto left = parseAdd();
            while (match(TokenKind::LT) || match(TokenKind::LE) || match(TokenKind::GT) || match(TokenKind::GE))
            {
                std::string op = previous().lexeme;
                left = binary(op, std::move(left), parseAdd(), previous().loc);
            }
            return left;
        }

        std::unique_ptr<ASTNode> parseAdd()
        {
            auto left = parseMul();
            while (match(TokenKind::PLUS) || match(TokenKind::MINUS))
            {
                std::string op = previous().lexeme;
                left = binary(op, std::move(left), parseMul(), previous().loc);
            }
            return left;
        }

        std::unique_ptr<ASTNode> parseMul()
        {
            auto left = parseUnary();
            while (match(TokenKind::STAR) || match(TokenKind::SLASH) || match(TokenKind::PERCENT))
            {
                std::string op = previous().lexeme;
                left = binary(op, std::move(left), parseUnary(), previous().loc);
            }
            return left;
        }

        std::unique_ptr<ASTNode> parseUnary()
        {
            if (match(TokenKind::NOT) || match(TokenKind::MINUS))
            {
                std::string op = previous().lexeme;
                auto node = makeNode(ASTKind::UnaryExpr, op, previous().loc);
                node->children.push_back(parseUnary());
                return node;
            }
            return parsePrimary();
        }

        std::unique_ptr<ASTNode> parsePrimary()
        {
            if (match(TokenKind::INT_LITERAL))
            {
                auto node = makeNode(ASTKind::IntLiteral, previous().lexeme, previous().loc);
                node->type = TypeKind::Int;
                return node;
            }
            if (match(TokenKind::KW_TRUE) || match(TokenKind::KW_FALSE))
            {
                auto node = makeNode(ASTKind::BoolLiteral, previous().lexeme, previous().loc);
                node->type = TypeKind::Bool;
                return node;
            }
            if (match(TokenKind::IDENT))
                return makeNode(ASTKind::Identifier, previous().lexeme, previous().loc);
            if (match(TokenKind::LPAREN))
            {
                auto expr = parseExpr();
                consume(TokenKind::RPAREN, "expected ')' after expression");
                return expr;
            }
            error(current().loc, "expected expression");
            auto node = makeNode(ASTKind::IntLiteral, "0", current().loc);
            node->type = TypeKind::Error;
            return node;
        }

        std::unique_ptr<ASTNode> binary(const std::string &op, std::unique_ptr<ASTNode> left,
                                        std::unique_ptr<ASTNode> right, SourceLocation loc)
        {
            auto node = makeNode(ASTKind::BinaryExpr, op, loc);
            node->children.push_back(std::move(left));
            node->children.push_back(std::move(right));
            return node;
        }
    };

    void printAST(std::ostream &out, const ASTNode *node, int depth = 0)
    {
        out << std::string(depth * 2, ' ') << astName(node->kind);
        if (!node->text.empty())
            out << " " << node->text;
        if (node->type != TypeKind::Error)
            out << " : " << typeName(node->type);
        out << "\n";
        for (const auto &child : node->children)
            printAST(out, child.get(), depth + 1);
    }

    class DotPrinter
    {
    public:
        std::string print(const ASTNode *root)
        {
            out_ << "digraph AST {\n  node [shape=box, fontname=\"Consolas\"];\n";
            visit(root);
            out_ << "}\n";
            return out_.str();
        }

    private:
        std::ostringstream out_;
        int nextId_ = 0;

        int visit(const ASTNode *node)
        {
            int id = nextId_++;
            std::string label = astName(node->kind);
            if (!node->text.empty())
                label += "\\n" + node->text;
            out_ << "  n" << id << " [label=\"" << label << "\"];\n";
            for (const auto &child : node->children)
            {
                int cid = visit(child.get());
                out_ << "  n" << id << " -> n" << cid << ";\n";
            }
            return id;
        }
    };

    class Semantic
    {
    public:
        void analyze(ASTNode *root)
        {
            enterScope();
            for (auto &child : root->children)
                analyzeFunction(child.get());
            exitScope();
        }

        const std::vector<CompileError> &errors() const { return errors_; }

        void printSymbols(std::ostream &out) const
        {
            out << "Scope  Kind      Type   Name\n";
            for (const auto &s : allSymbols_)
            {
                out << std::setw(5) << s.scopeLevel << "  "
                    << std::setw(8) << (s.kind == SymbolKind::Function ? "function" : "variable") << "  "
                    << std::setw(5) << typeName(s.type) << "  " << s.name << "\n";
            }
        }

    private:
        std::vector<std::unordered_map<std::string, Symbol>> scopes_;
        std::vector<Symbol> allSymbols_;
        std::vector<CompileError> errors_;
        TypeKind currentReturn_ = TypeKind::Void;
        int loopDepth_ = 0;

        void enterScope() { scopes_.push_back({}); }
        void exitScope() { scopes_.pop_back(); }
        int scopeLevel() const { return static_cast<int>(scopes_.size()) - 1; }

        void error(SourceLocation loc, const std::string &message)
        {
            errors_.push_back({"SemanticError", loc, message});
        }

        bool declare(const Symbol &symbol)
        {
            auto &scope = scopes_.back();
            if (scope.count(symbol.name))
                return false;
            scope[symbol.name] = symbol;
            allSymbols_.push_back(symbol);
            return true;
        }

        Symbol *lookup(const std::string &name)
        {
            for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
            {
                auto found = it->find(name);
                if (found != it->end())
                    return &found->second;
            }
            return nullptr;
        }

        void analyzeFunction(ASTNode *fn)
        {
            Symbol symbol{fn->text, SymbolKind::Function, fn->type, scopeLevel(), fn->loc};
            if (!declare(symbol))
                error(fn->loc, "function '" + fn->text + "' is already declared");
            currentReturn_ = fn->type;
            analyzeBlock(fn->children[0].get(), false);
        }

        void analyzeBlock(ASTNode *block, bool nested = true)
        {
            if (nested)
                enterScope();
            for (auto &child : block->children)
                analyzeStmt(child.get());
            if (nested)
                exitScope();
        }

        void analyzeStmt(ASTNode *node)
        {
            switch (node->kind)
            {
            case ASTKind::Block:
                analyzeBlock(node);
                break;
            case ASTKind::VarDecl:
            {
                Symbol symbol{node->text, SymbolKind::Variable, node->type, scopeLevel(), node->loc};
                if (!declare(symbol))
                    error(node->loc, "variable '" + node->text + "' is already declared in this scope");
                break;
            }
            case ASTKind::AssignStmt:
            {
                Symbol *s = lookup(node->text);
                TypeKind rhs = analyzeExpr(node->children[0].get());
                if (!s)
                    error(node->loc, "variable '" + node->text + "' is not declared");
                else if (rhs != TypeKind::Error && s->type != rhs)
                {
                    error(node->loc, "cannot assign " + typeName(rhs) + " to " + typeName(s->type) + " variable '" + node->text + "'");
                }
                break;
            }
            case ASTKind::ReadStmt:
            {
                ASTNode *id = node->children[0].get();
                Symbol *s = lookup(id->text);
                if (!s)
                    error(id->loc, "variable '" + id->text + "' is not declared");
                else if (s->type != TypeKind::Int)
                    error(id->loc, "read currently supports int variables only");
                break;
            }
            case ASTKind::WriteStmt:
                analyzeExpr(node->children[0].get());
                break;
            case ASTKind::IfStmt:
            {
                TypeKind cond = analyzeExpr(node->children[0].get());
                if (cond != TypeKind::Bool && cond != TypeKind::Error)
                    error(node->children[0]->loc, "if condition must be bool");
                analyzeStmt(node->children[1].get());
                if (node->children.size() > 2)
                    analyzeStmt(node->children[2].get());
                break;
            }
            case ASTKind::WhileStmt:
            {
                TypeKind cond = analyzeExpr(node->children[0].get());
                if (cond != TypeKind::Bool && cond != TypeKind::Error)
                    error(node->children[0]->loc, "while condition must be bool");
                ++loopDepth_;
                analyzeStmt(node->children[1].get());
                --loopDepth_;
                break;
            }
            case ASTKind::BreakStmt:
                if (loopDepth_ == 0)
                    error(node->loc, "break must be inside a loop");
                break;
            case ASTKind::ContinueStmt:
                if (loopDepth_ == 0)
                    error(node->loc, "continue must be inside a loop");
                break;
            case ASTKind::ReturnStmt:
            {
                TypeKind actual = analyzeExpr(node->children[0].get());
                if (actual != TypeKind::Error && actual != currentReturn_)
                    error(node->loc, "return type should be " + typeName(currentReturn_));
                break;
            }
            default:
                break;
            }
        }

        TypeKind analyzeExpr(ASTNode *node)
        {
            switch (node->kind)
            {
            case ASTKind::IntLiteral:
                node->type = TypeKind::Int;
                return node->type;
            case ASTKind::BoolLiteral:
                node->type = TypeKind::Bool;
                return node->type;
            case ASTKind::Identifier:
            {
                Symbol *s = lookup(node->text);
                if (!s)
                {
                    error(node->loc, "variable '" + node->text + "' is not declared");
                    node->type = TypeKind::Error;
                }
                else
                {
                    node->type = s->type;
                }
                return node->type;
            }
            case ASTKind::UnaryExpr:
            {
                TypeKind inner = analyzeExpr(node->children[0].get());
                if (node->text == "-")
                {
                    if (inner != TypeKind::Int && inner != TypeKind::Error)
                        error(node->loc, "unary '-' expects int");
                    node->type = inner == TypeKind::Error ? TypeKind::Error : TypeKind::Int;
                }
                else
                {
                    if (inner != TypeKind::Bool && inner != TypeKind::Error)
                        error(node->loc, "'!' expects bool");
                    node->type = inner == TypeKind::Error ? TypeKind::Error : TypeKind::Bool;
                }
                return node->type;
            }
            case ASTKind::BinaryExpr:
            {
                TypeKind left = analyzeExpr(node->children[0].get());
                TypeKind right = analyzeExpr(node->children[1].get());
                const std::string &op = node->text;
                if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%")
                {
                    if ((left != TypeKind::Int || right != TypeKind::Int) && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "operator '" + op + "' expects int operands");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Int;
                }
                else if (op == "<" || op == "<=" || op == ">" || op == ">=")
                {
                    if ((left != TypeKind::Int || right != TypeKind::Int) && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "operator '" + op + "' expects int operands");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
                }
                else if (op == "==" || op == "!=")
                {
                    if (left != right && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "equality operands should have same type");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
                }
                else
                {
                    if ((left != TypeKind::Bool || right != TypeKind::Bool) && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "operator '" + op + "' expects bool operands");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
                }
                return node->type;
            }
            default:
                return TypeKind::Error;
            }
        }
    };

    std::vector<VMInst> generateVM(const IRList &ir)
    {
        std::vector<VMInst> code;
        auto load = [&](const std::string &x)
        { code.push_back({isIntegerText(x) ? "PUSH" : "LOAD", x}); };
        for (const auto &q : ir)
        {
            switch (q.op)
            {
            case IROp::LABEL:
                code.push_back({"LABEL", q.result});
                break;
            case IROp::GOTO:
                code.push_back({"JMP", q.result});
                break;
            case IROp::IF_FALSE:
                load(q.arg1);
                code.push_back({"JZ", q.result});
                break;
            case IROp::ASSIGN:
                load(q.arg1);
                code.push_back({"STORE", q.result});
                break;
            case IROp::READ:
                code.push_back({"READ", q.result});
                break;
            case IROp::WRITE:
                load(q.arg1);
                code.push_back({"PRINT", ""});
                break;
            case IROp::RETURN:
                load(q.arg1);
                code.push_back({"RET", ""});
                break;
            case IROp::NOT:
                load(q.arg1);
                code.push_back({"NOT", ""});
                code.push_back({"STORE", q.result});
                break;
            case IROp::NEG:
                load(q.arg1);
                code.push_back({"NEG", ""});
                code.push_back({"STORE", q.result});
                break;
            default:
                load(q.arg1);
                load(q.arg2);
                code.push_back({irOpName(q.op), ""});
                code.push_back({"STORE", q.result});
                break;
            }
        }
        code.push_back({"HALT", ""});
        return code;
    }

    void printVM(std::ostream &out, const std::vector<VMInst> &code)
    {
        for (const auto &inst : code)
        {
            out << inst.op;
            if (!inst.arg.empty())
                out << " " << inst.arg;
            out << "\n";
        }
    }

    int runVM(const std::vector<VMInst> &code, std::istream &in, std::ostream &out)
    {
        std::unordered_map<std::string, int> vars;
        std::unordered_map<std::string, size_t> labels;
        std::vector<int> stack;
        for (size_t i = 0; i < code.size(); ++i)
            if (code[i].op == "LABEL")
                labels[code[i].arg] = i;

        auto pop = [&]()
        {
            int v = stack.empty() ? 0 : stack.back();
            if (!stack.empty())
                stack.pop_back();
            return v;
        };

        for (size_t pc = 0; pc < code.size(); ++pc)
        {
            const auto &inst = code[pc];
            if (inst.op == "PUSH")
                stack.push_back(std::stoi(inst.arg));
            else if (inst.op == "LOAD")
                stack.push_back(vars[inst.arg]);
            else if (inst.op == "STORE")
                vars[inst.arg] = pop();
            else if (inst.op == "READ")
            {
                int v = 0;
                in >> v;
                vars[inst.arg] = v;
            }
            else if (inst.op == "PRINT")
                out << pop() << "\n";
            else if (inst.op == "+")
            {
                int b = pop(), a = pop();
                stack.push_back(a + b);
            }
            else if (inst.op == "-")
            {
                int b = pop(), a = pop();
                stack.push_back(a - b);
            }
            else if (inst.op == "*")
            {
                int b = pop(), a = pop();
                stack.push_back(a * b);
            }
            else if (inst.op == "/")
            {
                int b = pop(), a = pop();
                stack.push_back(b == 0 ? 0 : a / b);
            }
            else if (inst.op == "%")
            {
                int b = pop(), a = pop();
                stack.push_back(b == 0 ? 0 : a % b);
            }
            else if (inst.op == "<")
            {
                int b = pop(), a = pop();
                stack.push_back(a < b);
            }
            else if (inst.op == "<=")
            {
                int b = pop(), a = pop();
                stack.push_back(a <= b);
            }
            else if (inst.op == ">")
            {
                int b = pop(), a = pop();
                stack.push_back(a > b);
            }
            else if (inst.op == ">=")
            {
                int b = pop(), a = pop();
                stack.push_back(a >= b);
            }
            else if (inst.op == "==")
            {
                int b = pop(), a = pop();
                stack.push_back(a == b);
            }
            else if (inst.op == "!=")
            {
                int b = pop(), a = pop();
                stack.push_back(a != b);
            }
            else if (inst.op == "&&")
            {
                int b = pop(), a = pop();
                stack.push_back(a && b);
            }
            else if (inst.op == "||")
            {
                int b = pop(), a = pop();
                stack.push_back(a || b);
            }
            else if (inst.op == "NOT")
                stack.push_back(!pop());
            else if (inst.op == "NEG")
                stack.push_back(-pop());
            else if (inst.op == "JMP")
                pc = labels[inst.arg];
            else if (inst.op == "JZ")
            {
                if (!pop())
                    pc = labels[inst.arg];
            }
            else if (inst.op == "RET")
                return pop();
            else if (inst.op == "HALT")
                return 0;
        }
        return 0;
    }

    std::string generatePseudoX86(const IRList &ir)
    {
        std::ostringstream out;
        out << "; Mini-C pseudo x86-64 output for presentation\n";
        for (const auto &q : ir)
        {
            if (q.op == IROp::LABEL)
                out << q.result << ":\n";
            else if (q.op == IROp::GOTO)
                out << "  jmp " << q.result << "\n";
            else if (q.op == IROp::IF_FALSE)
                out << "  cmp [" << q.arg1 << "], 0\n  je " << q.result << "\n";
            else if (q.op == IROp::ASSIGN)
                out << "  mov [" << q.result << "], " << q.arg1 << "\n";
            else if (q.op == IROp::READ)
                out << "  call read_int ; -> " << q.result << "\n";
            else if (q.op == IROp::WRITE)
                out << "  call print_int ; " << q.arg1 << "\n";
            else if (q.op == IROp::RETURN)
                out << "  mov rax, " << q.arg1 << "\n  ret\n";
            else
                out << "  ; " << q.result << " = " << q.arg1 << " " << irOpName(q.op) << " " << q.arg2 << "\n";
        }
        return out.str();
    }

    struct Pipeline
    {
        TokenList tokens;
        std::unique_ptr<ASTNode> ast;
        Semantic semantic;
        IRList ir;
        IRList optIr;
    };

    bool printErrors(const std::vector<CompileError> &errors)
    {
        for (const auto &e : errors)
            std::cerr << e.str() << "\n";
        return !errors.empty();
    }

    Pipeline compileFrontend(const std::string &source)
    {
        Pipeline p;
        Lexer lexer(source);
        p.tokens = lexer.scan();
        if (printErrors(lexer.errors()))
            throw std::runtime_error("lexical analysis failed");

        Parser parser(p.tokens);
        p.ast = parser.parseProgram();
        if (printErrors(parser.errors()))
            throw std::runtime_error("syntax analysis failed");

        p.semantic.analyze(p.ast.get());
        if (printErrors(p.semantic.errors()))
            throw std::runtime_error("semantic analysis failed");

        IRGenerator gen;
        p.ir = gen.generate(p.ast.get());
        p.optIr = optimizeIR(p.ir);
        return p;
    }

    void help()
    {
        std::cout
            << "Mini-C Compiler\n"
            << "Usage:\n"
            << "  compiler <input.mc> [--tokens|--ast|--dot|--check|--ir|--vm|--run|-S] [--opt]\n"
            << "Options:\n"
            << "  --tokens   print token stream\n"
            << "  --ast      print AST tree\n"
            << "  --dot      print Graphviz DOT for AST\n"
            << "  --check    run semantic check and print symbol table\n"
            << "  --ir       print three-address IR; add --opt for optimized IR\n"
            << "  --vm       print stack VM code\n"
            << "  --run      compile and run on VM\n"
            << "  -S         print presentation-friendly pseudo x86-64 assembly\n";
    }

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        help();
        return argc < 2 ? 1 : 0;
    }

    std::string path = argv[1];
    std::set<std::string> options;
    for (int i = 2; i < argc; ++i)
        options.insert(argv[i]);

    try
    {
        std::string source = readFile(path);
        bool onlyTokens = options.count("--tokens") > 0;

        if (onlyTokens)
        {
            Lexer lexer(source);
            TokenList tokens = lexer.scan();
            printTokens(std::cout, tokens);
            return printErrors(lexer.errors()) ? 1 : 0;
        }

        Pipeline p = compileFrontend(source);

        if (options.count("--ast"))
        {
            printAST(std::cout, p.ast.get());
        }
        else if (options.count("--dot"))
        {
            DotPrinter printer;
            std::cout << printer.print(p.ast.get());
        }
        else if (options.count("--check"))
        {
            std::cout << "semantic check passed\n";
            p.semantic.printSymbols(std::cout);
        }
        else if (options.count("--ir"))
        {
            printIR(std::cout, options.count("--opt") ? p.optIr : p.ir);
        }
        else if (options.count("--vm"))
        {
            auto vm = generateVM(options.count("--opt") ? p.optIr : p.ir);
            printVM(std::cout, vm);
        }
        else if (options.count("--run"))
        {
            auto vm = generateVM(p.optIr);
            return runVM(vm, std::cin, std::cout);
        }
        else if (options.count("-S"))
        {
            std::cout << generatePseudoX86(p.optIr);
        }
        else
        {
            std::cout << "compile passed. Use --help to see display options.\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
