#ifndef SHELL_H
#define SHELL_H

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

/*HERE ARE THE MACROS YOU CAN CHANGE THEM TO YOUR PREFERENCES*/
#define USERNAME "skinwalker"
#define PASSWORD 3654

typedef enum {
    TOKEN_SET,
    TOKEN_CALC,
    TOKEN_ERR,
    TOKEN_CUSTOM,
    TOKEN_NAME,
    TOKEN_STRING,
    TOKEN_SQUARE,
    TOKEN_EQUAL,
    TOKEN_DOLLAR,
    TOKEN_VALUE,
    TOKEN_PLUS,
    TOKEN_MINU,
    TOKEN_MULT,
    TOKEN_DIVI,
    TOKEN_POWE,
    TOKEN_WRITE,
    TOKEN_EOF,
} TokenType;

typedef struct Token {
    TokenType type;
    char name[40];
    float value;
    char stringValue[100];
} Token;

Token getNextToken(char **input);
typedef struct Variable {
    char name[100][40];
    float value[100];
    char stringValue[100][40];
    TokenType type[100];
    int counter;
} Variable;

void save_to_file(char *fileName,Variable *ptr);
void parseTokens(char **input,Variable *var);
int load_from_file(char *fileName,Variable *ptr);
void delete_variable(Variable *var,char **input);

typedef struct Expr {
    float valuevar1;
    char op[10];
    float valuevar2;
    struct {
        float square_value;
        char varName[100];
    } Squares;
    int isName; //0 = name | 1 = number
    TokenType type;
} Expr;

int parseSquare(char **input,Variable *var,Token token,Expr *obj);
Expr parseExpr(char **input,Variable *var);
void show_file_content(char *fileName);
void help_command(void);
void print_calc_command(void);
void print_fetch(void);
char *find_pwd(void);

#endif
