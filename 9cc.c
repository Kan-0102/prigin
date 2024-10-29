#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

Token *token; //現在着目しているトークン
char *user_input;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs){
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val){
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *expr(){
    Node *node = equality();
    return node;
}

Node *equality(){
    Node *node = relational();
    for(;;){
        if(consume("=="))
          node = new_node(ND_EQ, node, relational());
        else if(consume("!="))
          node = new_node(ND_NE, node, relational());
        else  
          return node;
    }
}

Node *relational(){
    Node *node = add();
    for(;;){
        if(consume(LT))
          node = new_node(ND_LT, node, add());
        else if(consume(LE))
          node = new_node(ND_LE, node, add());
        else if(consume(GT))
          node = new_node(ND_LT, add(), node);
        else if(consume(GE))
          node = new_node(ND_LE, add(), node);
        else  
          return node;
    }
}


Node *add(){
    Node *node = mul();
    for(;;){
        if(consume(ADD))
          node = new_node(ND_ADD, node, mul());
        else if(consume(SUB))
          node = new_node(ND_SUB, node, mul());
        else
          return node;
    }
}

Node *mul(){
    Node *node = unary();

    for(;;){
        if (consume(MUL))
          node = new_node(ND_MUL, node, unary());
        else if(consume(DIV))
          node = new_node(ND_DIV, node, unary());
        else
          return node;
    }
}

Node *unary(){
    if(consume(ADD))
      return primary();
    if(consume(SUB))
      return new_node(ND_SUB, new_node_num(0), primary());
    return primary();
}

Node *primary(){
    //次のトークンが"("なら"(" expr ")"のはず
    if(consume(LPARE)){
        Node *node = expr();
        expect(RPARE);
        return node;
    }

    //そうでなければ数値のはず
    return new_node_num(expect_number());
}

void gen(Node *node){
    if(node->kind == ND_NUM){
        printf(" push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf(" pop rdi\n");
    printf(" pop rax\n");

    switch (node->kind){
    case ND_EQ:
      printf(" cmp rax, rdi\n");
      printf(" sete al\n");
      printf(" movzb rax, al\n");
      break;
    case ND_NE:
      printf(" cmp rax, rdi\n");
      printf(" setne al\n");
      printf(" movzb rax, al\n");
      break;
    case ND_LT:
      printf(" cmp rax, rdi\n");
      printf(" setl al\n");
      printf(" movzb rax, al\n");
      break;
    case ND_LE:
      printf(" cmp rax, rdi\n");
      printf(" setle al\n");
      printf(" movzb rax, al\n");
      break;
    case ND_ADD:
      printf(" add rax, rdi\n");
      break;
    case ND_SUB:
      printf(" sub rax, rdi\n");
      break;
    case ND_MUL:
      printf(" imul rax, rdi\n");
      break;
    case ND_DIV:
      printf(" cqo\n");
      printf(" idiv rdi\n");
      break;
    }

    printf(" push rax\n");
}

void error_at(char *loc, char *fmt, ...){
    va_list ap; //va_list調べる
    va_start(ap, fmt);
    
    int pos = loc - user_input; //現在の文字アドレスから入力文字列のアドレスを引くことで文字数を出す
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " "); //その文字数分空白を開ける
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// //エラーを報告するための関数　printfと同じ引数を取る
// void error(char *fmt, ...){ //...は可変長引数で，任意の数の引数を渡せる
//     va_list ap; //可変長引数を格納するオブジェクト
//     va_start(ap, fmt);  //リストの初期化　ap:可変長引数リストを格納する　fmt:固定引数　可変長引数の開始位置
//     vfprintf(stderr, fmt, ap);  
//     fprintf(stderr, "\n");
//     exit(1);
// }

//次のトークンが期待している記号の時には，トークンを1つ読み進めて真を返す．それ以外の場合偽を返す．
bool consume(char *op){
    if (token->kind != TK_RESERVED ||  strlen(op) != token->len || memcmp(token->str, op, token->len))
      return false;
    token = token->next;
    return true;
}

//次のトークンが記号の場合，トークンを1つ読み進める．それ以外の場合エラーを報告．
void expect(char *op){
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
      error_at(token->str,"'%c'ではありません", op);
    token = token->next;
}

//次のトークンが数値の場合，トークンを1つ読み進めてその数値を返す．それ以外の場合エラーを報告．
int expect_number(){
    if (token->kind != TK_NUM)
      error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(){
    return token->kind == TK_EOF;
}

//新しいトークンを作成してcur(current)につなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len){
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind; 
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
} 

//入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p){
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while(*p){
        //空白文字をスキップ
        if(isspace(*p)){ //isspace:空白文字か否か調べる
            p++; 
            continue;
        }

        if(strncmp(p, ">=", 2) == 0 || strncmp(p, "<=", 2) == 0 || strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0){
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>'){
            cur = new_token(TK_RESERVED, cur, p++, 1); //その文字のトークンを作成し，前のcurにつなげる．今のtokはcurに代入
            continue;
        }

        if(isdigit(*p)){
            cur = new_token(TK_NUM, cur, p, 0); 
            cur->val = strtol(p, &p, 10);
            continue;
        }
        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

int main(int argc, char **argv){
    if (argc != 2){
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    //トークナイズしてパースする
    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    //抽象構文木を下りながらコード生成
    gen(node);

    //スタックトップに式全体の値が残っているはずなのでそれをRAXにロードして関数からの返り値とする
    printf(" pop rax\n");
    printf(" ret\n");

    return 0;
}