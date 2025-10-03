#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

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
    TOKEN_NAME,
    TOKEN_STRING,
    TOKEN_EQUAL,
    TOKEN_DOLLAR,
    TOKEN_VALUE,
    TOKEN_PLUS,
    TOKEN_MINU,
    TOKEN_MULT,
    TOKEN_DIVI,
    TOKEN_POWE,
    TOKEN_EOF,
} TokenType;

typedef struct Token {
    TokenType type;
    char name[40];
    float value;
    char stringValue[100];
} Token;

Token getNextToken(char **input) {
    while (isspace(**input)) (*input)++;

    if(isalpha(**input)) {
        Token token;
        int counter = 0;
        while(isalnum(**input) || **input=='.') {
            token.name[counter++] = **input;
            (*input)++;
        }

        token.name[counter] = '\0';
        if(strcmp(token.name, "set") == 0) {
            token.type = TOKEN_SET;
            return token;
        } else if(strcmp(token.name,"calc")==0) {
            token.type = TOKEN_CALC;
            return token;
        } else {
            token.type = TOKEN_NAME;
            return token;
        }
    }

    if(isdigit(**input)) {
        Token token = {TOKEN_VALUE,""};
        int counter = 0;
        char name[100];
        while(isdigit(**input) || **input == '.') {
            name[counter++] = **input;
            (*input)++;
        }

        name[counter] = '\0';
        char *endPtr;

        float number = strtof(name,&endPtr);
        if(*endPtr != '\0') {
            printf(RED"Error: Invalid number '%s'\n"RESET,name);
            return (Token){TOKEN_ERR,""};
        }

        token.value = number;
        return token;
    }

    if(**input == '"') {
        Token token = {TOKEN_STRING,""};
        int counter = 0;

        (*input)++;
        while(**input != '"' && **input != '\0') {
            token.stringValue[counter] = **input;
            (*input)++;
            counter++;
        }

        token.stringValue[counter] = '\0';
        (*input)++;

        return token;
    }

    char operation = **input;
    (*input)++;

    switch(operation) {
        case '+': return (Token){TOKEN_PLUS,"+"};
        case '-': return (Token){TOKEN_MINU,"-"};
        case '*': return (Token){TOKEN_MULT,"*"};
        case '/': return (Token){TOKEN_DIVI,"/"};
        case '^': return (Token){TOKEN_POWE,"^"};
        case '=': return (Token){TOKEN_EQUAL,"="};
        case '$': return (Token){TOKEN_DOLLAR,"$"};
    }

    return (Token){TOKEN_EOF, ""};
}

typedef struct Variable {
    char name[100][40];
    float value[100];
    char stringValue[100][40];
    TokenType type[100];
    int counter;
} Variable;

void parseTokens(char **input, Variable *var) {
    Token token = getNextToken(input);
    int idx = var->counter;

    if(token.type != TOKEN_SET) {
        printf(RED"Error: 'set' is missing\n"RESET);
        return;
    }

    token = getNextToken(input);
    if(token.type != TOKEN_NAME) {
        printf(RED"Error: Invalid variable name\n"RESET);
        return;
    }

    int foundIdx = -1;
    for(int i=0; i<var->counter; i++) {
        if(strcmp(var->name[i],token.name)==0) {
            foundIdx = 1;
            break;
        }
    }

    if(foundIdx != -1) {
        printf(RED"Error: This variable name already exists\n"RESET);
        return;
    }

    strcpy(var->name[idx], token.name);
    token = getNextToken(input);
    if(token.type != TOKEN_EQUAL) {
        printf(RED"Error: '=' is missing\n"RESET);
        return;
    }

    token = getNextToken(input);
    if(token.type != TOKEN_VALUE && token.type != TOKEN_STRING) {
        printf(RED"Error: Invalid variable value\n"RESET);
        return;
    } else {
        if(token.type == TOKEN_VALUE) {
            var->value[idx] = token.value;
            var->type[idx] = TOKEN_VALUE;
        } else if(token.type == TOKEN_STRING) {
            strcpy(var->stringValue[idx],token.stringValue);
            var->type[idx] = TOKEN_STRING;
        } else {
            printf(RED"Error: Strings need \"\"\n"RESET);
            return;
        }
    }

    token = getNextToken(input);
    if(token.type != TOKEN_EOF) {
        printf(RED"Error: Invalid arguments count passed\n"RESET);
        return;
    }

    var->counter++;
}

void delete_variable(Variable *var,char **input) {
    if(var->counter == 0) {
        printf(RED"Error: List is empty\n"RESET);
        return;
    }

    Token token = getNextToken(input);
    if(strcmp(token.name,"delete")!=0) {
        printf(RED"Error: Invalid command '%s'\n"RESET,token.name);
        return;
    }

    token = getNextToken(input);

    int index = -1;
    int foundIdx = -1;
    for(int i=0; i<var->counter; i++) {
        if(strcmp(var->name[i],token.name)==0) {
            foundIdx = 1;
            index = i;
            break;
        }
    }

    if(foundIdx == -1) {
        printf(RED"Error: Variable with name '%s' not found\n"RESET,token.name);
        return;
    }

    if(var->type[index] == TOKEN_VALUE) {
        for(int i=index; i<var->counter-1; i++) {
            strcpy(var->name[i],var->name[i+1]); 
            var->value[i] = var->value[i+1];
            var->type[i] = var->type[i+1];
        }
    } else if(var->type[index] == TOKEN_STRING) {
        for(int i=index; i<var->counter-1; i++) {
            strcpy(var->name[i],var->name[i+1]);
            strcpy(var->stringValue[i],var->stringValue[i+1]);
            var->type[i] = var->type[i+1];
        }
    }

    var->counter--;
    printf(GREEN"Variable deleted succesfully\n"RESET);
}

typedef struct Expr {
    float valuevar1;
    char op[10];
    float valuevar2;
    TokenType type;
} Expr;

Expr parseExpr(char **input,Variable *var) {
    Expr obj;
    
    Token token = getNextToken(input);
    if(token.type != TOKEN_CALC) {
        printf(RED"Error: 'calc' is missing\n"RESET);
        return (Expr){.type=TOKEN_ERR};
    }

    token = getNextToken(input);
    if(token.type == TOKEN_NAME) {
        int foundIdx1 = -1; 
        float variable_value1;
        for(int i=0; i<var->counter; i++) {
            if(strcmp(var->name[i],token.name)==0) {
                if(var->type[i] == TOKEN_STRING) {
                    printf(RED"Error: Cannot do operations with strings\n"RESET);
                    return (Expr){.type=TOKEN_ERR};
                }

                foundIdx1 = 1;
                variable_value1 = var->value[i];
                break;
            }
        }

        if(foundIdx1 == -1) {
            printf(RED"Error: Variable with name '%s' not found\n"RESET,token.name);
            return (Expr){.type=TOKEN_ERR};
        }

        obj.valuevar1 = variable_value1;
    } else if(token.type == TOKEN_VALUE) {
        obj.valuevar1 = token.value;
    } else {
        printf(RED"Error: Invalid variable name '%s'\n"RESET,token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    token = getNextToken(input);
    if(token.type != TOKEN_PLUS && 
       token.type != TOKEN_MINU &&
       token.type != TOKEN_MULT &&
       token.type != TOKEN_DIVI &&
       token.type != TOKEN_POWE) {
        printf(RED"Error: Invalid operation '%s'\n"RESET,token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    strcpy(obj.op,token.name);

    token = getNextToken(input);
    if(token.type == TOKEN_NAME) {
        int foundIdx2 = -1;
        float variable_value2;
        for(int i=0; i<var->counter; i++) {
            if(strcmp(var->name[i],token.name)==0) {
                if(var->type[i] == TOKEN_STRING) {
                    printf(RED"Error: Cannot do operations with strings\n"RESET);
                    return (Expr){.type=TOKEN_ERR};
                }

                foundIdx2 = 1;
                variable_value2 = var->value[i];
                break;
            }
        }

        if(foundIdx2 == -1) {
            printf(RED"Error: Variable with name '%s' not found\n"RESET,token.name);
            return (Expr){.type=TOKEN_ERR};
        }

        obj.valuevar2 = variable_value2;
    } else if(token.type == TOKEN_VALUE) {
        obj.valuevar2 = token.value;
    } else {
        printf(RED"Error: Invalid variable name '%s'\n"RESET,token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    token = getNextToken(input);
    if(token.type != TOKEN_EOF) {
        printf(RED"Error: Invalid arguments count passed\n"RESET);
        return (Expr){.type=TOKEN_ERR};
    }

    return obj;
}

void show_file_content(char *filename) {
    FILE *file = fopen(filename,"r");
    if(!file) {
        printf(RED"Error: Failed to open this file\n"RESET);
        return;
    }

    char ch;
    while((ch = getc(file))!=EOF) 
        printf("%c",ch);
    fclose(file);
}

void help_command() {
    printf("\nCommands:\n");
    printf(CYAN"  set <name>=<value>        | creates variables\n"RESET);
    printf(CYAN"  echo <string>             | prints a string\n"RESET);
    printf(CYAN"  echo $<variable>          | prints the variable and his value\n"RESET);
    printf(CYAN"  calc <var1> <op> <var2>   | calculates the operation by the variables value\n"RESET);
    printf(CYAN"  calc <num1> <op> <num2>   | calculates the operation by the values you passed\n"RESET);
    printf(CYAN"  create <filename>         | creates a file\n"RESET);
    printf(CYAN"  rm <filename>             | deletes a file\n"RESET);
    printf(CYAN"  cd <directory>            | goes to a different directory\n"RESET);
    printf(CYAN"  cp <file1> <file2>        | copies the content of the first file to the second\n"RESET);
    printf(CYAN"  fsize <filename>          | it shows you the size of a file\n"RESET);
    printf(CYAN"  write <filename> <text>   | writes text into a file\n"RESET);
    printf(CYAN"  countvars                 | it shows you the total number of stored variables\n"RESET);
    printf(CYAN"  ls                        | prints directorys\n"RESET);
    printf(CYAN"  cls                       | clears the terminal screen\n"RESET);
    printf(CYAN"  info                      | prints systems information\n"RESET);
    printf(CYAN"  pwd                       | prints current working directory\n"RESET);
    printf(CYAN"  show                      | prints all the stored variables\n"RESET);
    printf(CYAN"  whoami                    | shows users shell name\n"RESET);
    printf(CYAN"  cat <file_name>           | shows files content\n"RESET);
    printf(CYAN"  delete <varname>          | deletes a variable\n"RESET);
    printf(CYAN"  help                      | shows this pannel\n"RESET);
    printf(CYAN"  exit                      | closes the program\n\n");
}

void print_neofetch() {
    printf("\n");
    printf(RED   "  ███╗   ██╗ █████╗  ██████╗ ███████╗ " RESET YELLOW "    | CPU: Intel i8 Chip      | OS: Linux 6.5\n");
    printf(GREEN "  ████╗  ██║██╔══██╗██╔════╝ ██╔════╝ " RESET BLUE   "    | GPU: RTX 4090           | Kernel: x86_64\n");
    printf(CYAN  "  ██╔██╗ ██║███████║██║  ███╗█████╗   " RESET MAGENTA"    | SSD: 1TB                | Shell: bash\n");
    printf(WHITE "  ██║╚██╗██║██╔══██║██║   ██║██╔══╝   " RESET YELLOW "    | RAM: 64GB               | DE: GNOME 45\n");
    printf(RED   "  ██║ ╚████║██║  ██║╚██████╔╝███████╗ " RESET CYAN   "    | NAME: Hacked_Linux      | Uptime: 12h 34m\n");
    printf(GREEN "  ╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝ " RESET BLUE   "    | Host: Custom-PC         | Packages: 432\n");
    printf("\n"); 
}

int main(void) {
    Variable var = {.counter = 0};
    char input[100];
    char command[50];
    Expr expr;

    while(1) {
        printf(BOLD CYAN"%s"RESET " " GREEN ">> " RESET,USERNAME);
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        if(strlen(input)==0) continue;
        sscanf(input, "%s", command);

        if(strcmp(command, "set") == 0) {
            char *ptr = input;
            parseTokens(&ptr, &var);
        } else if(strcmp(command,"calc")==0){
            char *ptr = input; 
            expr = parseExpr(&ptr,&var);

            if(expr.type == TOKEN_ERR) { continue; }
            if(strcmp(expr.op,"+")==0) {
                printf("%.2f\n",expr.valuevar1+expr.valuevar2);
            } else if(strcmp(expr.op,"-")==0) {
                printf("%.2f\n",expr.valuevar1-expr.valuevar2);
            } else if(strcmp(expr.op,"*")==0) {
                printf("%.2f\n",expr.valuevar1*expr.valuevar2);
            } else if(strcmp(expr.op,"/")==0) {
                if(expr.valuevar2 == 0) {
                    printf(RED"Error: Division by 0!\n"RESET);
                    continue;
                } else {
                    printf("%.2f\n",expr.valuevar1/expr.valuevar2);
                }
            } else if(strcmp(expr.op,"^")==0) {
                printf("%.2f\n",pow(expr.valuevar1,expr.valuevar2));
                continue;
            }
        } else if(strcmp(command, "echo") == 0) {
            char *ptr = input;
            getNextToken(&ptr);

            Token token = getNextToken(&ptr);
            if(token.type == TOKEN_DOLLAR) {
                if(var.counter == 0) {
                    printf(RED"Error: List is empty\n"RESET);
                    continue;
                }

                int indexIdx;
                int foundIdx = -1;

                token = getNextToken(&ptr);
                for(int i = 0; i < var.counter; i++) {
                    if(strcmp(var.name[i], token.name) == 0) {
                        indexIdx = i;
                        foundIdx = 1;
                        break;
                    }
                }

                if(foundIdx == -1) {
                    printf(RED"Error: Variable with the name '%s' not found\n"RESET,token.name);
                    continue;
                }

                token = getNextToken(&ptr);
                if(token.type != TOKEN_EOF) {
                    printf(RED"Error: Invalid arguments count passed\n"RESET);
                    continue;
                }

                if(var.type[indexIdx] == TOKEN_STRING) {
                    printf("%s = \"%s\"\n",var.name[indexIdx],var.stringValue[indexIdx]);
                    continue;
                } else {
                    printf("%s = %.2f\n",var.name[indexIdx],var.value[indexIdx]);
                    continue;
                }
            } else if(token.type == TOKEN_NAME) {
                printf("%s\n",token.name);
                continue;
            } else if(token.type == TOKEN_STRING) {
                printf("%s\n",token.stringValue);
                continue;
            } else {
                printf(RED"Error: '$' is missing\n"RESET);
                continue;
            }
        } else if(strcmp(input, "show") == 0) {
            if(var.counter == 0) {
                printf(RED"Error: List is empty\n"RESET);
                continue;
            }

            for(int i = 0; i < var.counter; i++) {
                if(var.type[i] == TOKEN_VALUE) {
                    printf(CYAN "%s"RESET " = " YELLOW "%.2f\n" RESET, var.name[i], var.value[i]);
                } else {
                    printf(CYAN "%s"RESET " = " MAGENTA "\"%s\"\n" RESET,var.name[i], var.stringValue[i]);
                } 
            }
        } else if(strcmp(command,"cat")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 2) {
                show_file_content(tokens[1]);
                continue;
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(command,"create")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(isdigit(tokens[1][0])) {
                printf(RED"Error: Cannot create a file with the first character a number\n"RESET);
                continue;
            }

            if(counter == 2) {
                char prompt[100] = "touch ";
                strcat(prompt,tokens[1]);
                system(prompt);
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(command,"rm")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 2) {
                int check = remove(tokens[1]);
                if(check == -1) {
                    printf(RED"Error: File not found\n"RESET);
                    continue;
                }

                printf(GREEN"File deleted succesfully\n"RESET);
                continue;
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(command,"cd")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 2) {
                if(strcmp(tokens[1],"~")==0) {
                    chdir("/home/skinwalker");
                    continue;
                }

                int check = chdir(tokens[1]);
                if(check == -1) {
                    printf(RED"Error: Invalid directory '%s'\n"RESET,tokens[1]);
                    continue;
                }
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(input,"pwd")==0) {
            long size = pathconf(".", _PC_PATH_MAX);
            if(size == -1) size = 4096;

            char *buff = malloc((size_t)size);
            if(buff == NULL) {
                printf(RED"Error: Could not allocate buffer\n"RESET);
                continue;
            }

            if(getcwd(buff, (size_t)size) != NULL) {
                printf(BLUE"%s\n"RESET, buff);
            } else {
                perror("getcwd");
            }

            free(buff);
        } else if(strcmp(command,"cp")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 3) {
                FILE *file1 = fopen(tokens[1], "r");
                if(file1 == NULL) {
                    printf(RED"Error: Failed to open source file '%s'\n"RESET, tokens[1]);
                    continue;
                }

                FILE *file2 = fopen(tokens[2], "w");
                if(file2 == NULL) {
                    printf(RED"Error: Failed to create destination file '%s'\n"RESET, tokens[2]);
                    fclose(file1);
                    continue;
                }

                int ch;
                while((ch = fgetc(file1)) != EOF) 
                    fputc(ch, file2);

                fclose(file1);
                fclose(file2);
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(command,"fsize")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 2) {
                FILE *file = fopen(tokens[1],"r");
                if(!file) {
                    printf(RED"Error: Failed to open the file\n"RESET);
                    continue;
                }

                char ch;
                int countChars = 0;
                while((ch = getc(file))!=EOF) countChars++;
                fclose(file);
                printf("Total bytes of \"%s\" -> %d\n",tokens[1],countChars);
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(command,"write")==0) {
            char *ptr = input;
            getNextToken(&ptr);

            Token token = getNextToken(&ptr);
            if(token.type != TOKEN_NAME) {
                printf(RED"Error: Invalid file name '%s'\n"RESET,token.name);
                continue;
            }

            FILE *file = fopen(token.name,"a");
            if(!file) {
                printf(RED"Error: Failed to open the file\n"RESET);
                continue;
            }

            token = getNextToken(&ptr);
            if(token.type != TOKEN_STRING) {
                printf("Error: Text must be in inside \"\"\n");
                fclose(file);
                continue;
            }

            fprintf(file,"%s\n",token.stringValue);
            fclose(file);

            token = getNextToken(&ptr);
            if(token.type != TOKEN_EOF) {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                fclose(file);
                continue;
            }

            printf(GREEN"File saved succesfully\n"RESET);
        } else if(strcmp(command,"delete")==0) {
            char *ptr = input;
            delete_variable(&var,&ptr);
        } else if(strcmp(input,"whoami")==0) {
            printf(BLUE"USER: "RESET CYAN"%s"RESET " | " BLUE "PASSWORD: "RESET CYAN"%d\n"RESET,USERNAME,PASSWORD);
        } else if(strcmp(input,"countvars")==0) {
            printf(BLUE"Total variables: %d\n"RESET,var.counter);
        } else if(strcmp(input,"cls")==0) {
            system("clear");
        } else if(strcmp(input,"ls")==0) {
            system("ls");
        } else if(strcmp(input,"help")==0) {
            help_command();
        } else if(strcmp(input,"info")==0) {
            print_neofetch();
        } else if(strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            return 0;
        } else {
            printf(RED"Error: Invalid command '%s'\n"RESET, input);
            continue;
        }
    }

    return 0;
}
