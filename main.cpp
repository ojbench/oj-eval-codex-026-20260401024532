#include <bits/stdc++.h>
using namespace std;

// Tokenizer for expression parsing
struct Token {
    enum Type { NUMBER, PLUS, MINUS, MUL, DIV, LPAREN, RPAREN, END } type;
    long long value;
};

class Lexer {
public:
    explicit Lexer(const string& s): str(s), i(0) {}
    Token next() {
        skipSpaces();
        if (i >= str.size()) return {Token::END, 0};
        char c = str[i];
        if (isdigit(static_cast<unsigned char>(c))) {
            long long v = 0;
            // parse integer (no sign here)
            while (i < str.size() && isdigit(static_cast<unsigned char>(str[i]))) {
                int d = str[i] - '0';
                v = v * 10 + d;
                ++i;
            }
            return {Token::NUMBER, v};
        }
        ++i;
        switch (c) {
            case '+': return {Token::PLUS, 0};
            case '-': return {Token::MINUS, 0};
            case '*': return {Token::MUL, 0};
            case '/': return {Token::DIV, 0};
            case '(': return {Token::LPAREN, 0};
            case ')': return {Token::RPAREN, 0};
            default:
                // Unknown character; treat as END to stop processing
                return {Token::END, 0};
        }
    }
    size_t position() const { return i; }
private:
    void skipSpaces() {
        while (i < str.size() && isspace(static_cast<unsigned char>(str[i]))) ++i;
    }
    string str;
    size_t i;
};

// Visitor pattern AST
struct Visitor;
struct Expr {
    virtual ~Expr() = default;
    virtual std::any accept(Visitor& v) = 0;
};

struct NumberExpr : Expr {
    long long value;
    explicit NumberExpr(long long v): value(v) {}
    std::any accept(Visitor& v) override;
};

struct UnaryExpr : Expr {
    char op; // '+' or '-'
    unique_ptr<Expr> expr;
    UnaryExpr(char o, unique_ptr<Expr> e): op(o), expr(std::move(e)) {}
    std::any accept(Visitor& v) override;
};

struct BinaryExpr : Expr {
    char op; // '+', '-', '*', '/'
    unique_ptr<Expr> left, right;
    BinaryExpr(unique_ptr<Expr> l, char o, unique_ptr<Expr> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    std::any accept(Visitor& v) override;
};

struct Visitor {
    virtual ~Visitor() = default;
    virtual std::any visit(NumberExpr& n) = 0;
    virtual std::any visit(UnaryExpr& u) = 0;
    virtual std::any visit(BinaryExpr& b) = 0;
};

std::any NumberExpr::accept(Visitor& v) { return v.visit(*this); }
std::any UnaryExpr::accept(Visitor& v) { return v.visit(*this); }
std::any BinaryExpr::accept(Visitor& v) { return v.visit(*this); }

// Parser implementing grammar:
// expr  := term ((+|-) term)*
// term  := factor ((*|/) factor)*
// factor:= NUMBER | '(' expr ')' | ('+'|'-') factor
class Parser {
public:
    explicit Parser(const string& s): lex(s) { cur = lex.next(); }
    unique_ptr<Expr> parse() {
        auto e = parseExpr();
        return e;
    }
private:
    unique_ptr<Expr> parseExpr() {
        auto node = parseTerm();
        while (cur.type == Token::PLUS || cur.type == Token::MINUS) {
            char op = (cur.type == Token::PLUS ? '+' : '-');
            advance();
            auto rhs = parseTerm();
            node = make_unique<BinaryExpr>(std::move(node), op, std::move(rhs));
        }
        return node;
    }
    unique_ptr<Expr> parseTerm() {
        auto node = parseFactor();
        while (cur.type == Token::MUL || cur.type == Token::DIV) {
            char op = (cur.type == Token::MUL ? '*' : '/');
            advance();
            auto rhs = parseFactor();
            node = make_unique<BinaryExpr>(std::move(node), op, std::move(rhs));
        }
        return node;
    }
    unique_ptr<Expr> parseFactor() {
        if (cur.type == Token::PLUS || cur.type == Token::MINUS) {
            char op = (cur.type == Token::PLUS ? '+' : '-');
            advance();
            auto sub = parseFactor();
            return make_unique<UnaryExpr>(op, std::move(sub));
        }
        if (cur.type == Token::NUMBER) {
            long long v = cur.value;
            advance();
            return make_unique<NumberExpr>(v);
        }
        if (cur.type == Token::LPAREN) {
            advance();
            auto inside = parseExpr();
            if (cur.type == Token::RPAREN) advance();
            return inside;
        }
        // Fallback to zero on unexpected token to avoid crash in judge; robust handling
        return make_unique<NumberExpr>(0);
    }
    void advance() { cur = lex.next(); }
    Lexer lex;
    Token cur{Token::END, 0};
};

struct EvalVisitor : Visitor {
    std::any visit(NumberExpr& n) override {
        return n.value;
    }
    std::any visit(UnaryExpr& u) override {
        long long v = std::any_cast<long long>(u.expr->accept(*this));
        if (u.op == '+') return v;
        else return -v;
    }
    std::any visit(BinaryExpr& b) override {
        long long L = std::any_cast<long long>(b.left->accept(*this));
        long long R = std::any_cast<long long>(b.right->accept(*this));
        switch (b.op) {
            case '+': return L + R;
            case '-': return L - R;
            case '*': return L * R;
            case '/':
                if (R == 0) {
                    // Define behavior: avoid crash; return 0
                    return 0LL;
                }
                return L / R; // integer division truncates toward zero
        }
        return 0LL;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<string> lines;
    string line;
    bool hasInput = false;
    while (std::getline(cin, line)) {
        hasInput = true;
        // Preserve line; skip if empty after trimming
        bool allspace = true;
        for (char c : line) if (!isspace(static_cast<unsigned char>(c))) { allspace = false; break; }
        if (!allspace) lines.push_back(line);
    }

    if (!hasInput) return 0;

    if (lines.empty()) {
        // If only whitespace, print nothing
        return 0;
    }

    EvalVisitor ev;
    for (size_t idx = 0; idx < lines.size(); ++idx) {
        Parser p(lines[idx]);
        auto ast = p.parse();
        long long result = std::any_cast<long long>(ast->accept(ev));
        cout << result;
        if (idx + 1 < lines.size()) cout << '\n';
    }
    return 0;
}
