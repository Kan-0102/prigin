#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//トークンの種類
typedef enum{
    TK_RESERVED,    //記号
    TK_NUM,         //整数トークン
    TK_EOF,         //入力の終わりを表すトークン
}TokenKind;

typedef struct Token Token;

// トークン型
struct Token{
    TokenKind kind; //トークンの型
    Token *next;    //次の入力トークン
    int val;        //kindがTK_NUMの場合，その数値
    char *str;      //トークン文字列
};

//現在着目しているトークン
Token *token;

char *user_input;

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

//エラーを報告するための関数　printfと同じ引数を取る
void error(char *fmt, ...){ //...は可変長引数で，任意の数の引数を渡せる
    va_list ap; //可変長引数を格納するオブジェクト
    va_start(ap, fmt);  //リストの初期化　ap:可変長引数リストを格納する　fmt:固定引数　可変長引数の開始位置
    vfprintf(stderr, fmt, ap);  
    fprintf(stderr, "\n");
    exit(1);
}

//次のトークンが期待している記号の時には，トークンを1つ読み進めて真を返す．それ以外の場合偽を返す．
bool consume(char op){
    if (token->kind != TK_RESERVED || token->str[0] != op)
      return false;
    token = token->next;
    return true;
}

//次のトークンが記号の場合，トークンを1つ読み進める．それ以外の場合エラーを報告．
void expect(char op){
    if (token->kind != TK_RESERVED || token->str[0] != op)
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
Token *new_token(TokenKind kind, Token *cur, char *str){
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind; 
    tok->str = str;
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

        if (*p == '+' || *p == '-'){
            cur = new_token(TK_RESERVED, cur, p++); //その文字のトークンを作成し，前のcurにつなげる．今のtokはcurに代入
            continue;
        }

        if(isdigit(*p)){
            cur = new_token(TK_NUM, cur, p); 
            cur->val = strtol(p, &p, 10);
            continue;
        }
        error("トークナイズできません");
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}


int main(int argc, char **argv){
    if (argc != 2){
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];

    //トークナイズする
    token = tokenize(user_input);

    //アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    //式の最初は数でなければならないのでそれをチェックして，最初のmov命令を出力
    printf(" mov rax, %d\n", expect_number());

    //'+ <数>'あるいは'- <数>'というトークンの並びを消費しつつアセンブリを出力
    while(!at_eof()){
        if(consume('+')){
            printf(" add rax, %d\n", expect_number());
            continue;
        }

        expect('-');
        printf(" sub rax, %d\n", expect_number());
    }

    printf(" ret\n");
    return 0;
}