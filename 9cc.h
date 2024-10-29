#ifndef _9CC_H
#define _9CC_H

#define ADD "+"
#define SUB "-"
#define MUL "*"
#define DIV "/"
#define EQ "=="
#define NE "!="
#define LT "<"
#define LE "<="
#define GT ">"
#define GE ">="
#define LPARE "("
#define RPARE ")"

typedef struct Node Node;
typedef struct Token Token;

//抽象構文木のノードの種類
typedef enum{
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // 整数
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_GT,  // >
    ND_GE,  // >=
}NodeKind;

//抽象構文木のノードの型
struct Node{
    NodeKind kind; // ノードの型
    Node *lhs; // 左辺(left-hand side)
    Node *rhs; // 右辺(right-hand side)
    int val; // kindがND_NUMの場合のみ使う
};

//トークンの種類
typedef enum{
    TK_RESERVED,    //記号
    TK_NUM,         //整数トークン
    TK_EOF,         //入力の終わりを表すトークン
}TokenKind;

// トークン型
struct Token{
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMの場合，その数値
    char *str;      //トークン文字列
    int len;        //トークンの長さ
};

//関数宣言
Node *expr();
Node *mul();
Node *unary();
Node *primary();

Node *equality();
Node *relational();
Node *add();

void gen(Node *node);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

#endif
