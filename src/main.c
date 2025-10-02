#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define USER_NAME "skinwalker"
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
    TOKEN_EOF_T,
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
        while(isalnum(**input)) {
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
            printf("Error: Invalid number '%s'\n",name);
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
        case '=': return (Token){TOKEN_EQUAL,"="};
        case '$': return (Token){TOKEN_DOLLAR,"$"};
    }

    return (Token){TOKEN_EOF_T, ""};
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
        printf("Error: 'set' is missing\n");
        return;
    }

    token = getNextToken(input);
    if(token.type != TOKEN_NAME) {
        printf("Error: Invalid variable name\n");
        return;
    }

    int foundIdx = -1;
    for(int i=0; i<var->counter; i++) {
        if(strcmp(var->name[i],token.name)==0) {
            foundIdx = 1;
            break;
        }
    }

    if(foundIdx == 1) {
        printf("Error: This variable name already exists\n");
        return;
    }

    strcpy(var->name[idx], token.name);
    token = getNextToken(input);
    if(token.type != TOKEN_EQUAL) {
        printf("Error: '=' is missing\n");
        return;
    }

    token = getNextToken(input);
    if(token.type != TOKEN_VALUE && token.type != TOKEN_STRING) {
        printf("Error: Invalid variable value\n");
        return;
    } else {
        if(token.type == TOKEN_VALUE) {
            var->value[idx] = token.value;
            var->type[idx] = TOKEN_VALUE;
        } else if(token.type == TOKEN_STRING) {
            strcpy(var->stringValue[idx],token.stringValue);
            var->type[idx] = TOKEN_STRING;
        } else {
            printf("Error: Strings need \"\"\n");
            return;
        }
    }

    token = getNextToken(input);
    if(token.type != TOKEN_EOF_T) {
        printf("Error: Invalid arguments count passed\n");
        return;
    }

    var->counter++;
}

void delete_variable(Variable *var,char **input) {
    if(var->counter == 0) {
        printf("Error: List is empty\n");
        return;
    }

    Token token = getNextToken(input);
    if(strcmp(token.name,"delete")!=0) {
        printf("Error: Invalid command '%s'\n",token.name);
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
        printf("Error: Variable with name '%s' not found\n",token.name);
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
    printf("Variable deleted succesfully\n");
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
        printf("Error: 'calc' is missing\n");
        return (Expr){.type=TOKEN_ERR};
    }

    token = getNextToken(input);
    if(token.type != TOKEN_NAME) {
        printf("Error: Invalid variable name '%s'\n",token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    int foundIdx1 = -1; 
    float variable_value1;

    for(int i=0; i<var->counter; i++) {
        if(strcmp(var->name[i],token.name)==0) {
            if(var->type[i] == TOKEN_STRING) {
                printf("Error: Cannot do operations with strings\n");
                return (Expr){.type=TOKEN_ERR};
            }

            foundIdx1 = 1;
            variable_value1 = var->value[i];
            break;
        }
    }

    if(foundIdx1 == -1) {
        printf("Error: Variable with name '%s' not found\n",token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    obj.valuevar1 = variable_value1;

    token = getNextToken(input);
    if(token.type != TOKEN_PLUS && token.type != TOKEN_MINU && token.type != TOKEN_MULT && token.type != TOKEN_DIVI) {
        printf("Error: Invalid operation '%s'\n",token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    strcpy(obj.op,token.name);

    token = getNextToken(input);
    if(token.type != TOKEN_NAME) {
        printf("Error: Invalid variable name '%s'\n",token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    int foundIdx2 = -1;
    float variable_value2;

    for(int i=0; i<var->counter; i++) {
        if(strcmp(var->name[i],token.name)==0) {
            if(var->type[i] == TOKEN_STRING) {
                printf("Error: Cannot do operations with strings\n");
                return (Expr){.type=TOKEN_ERR};
            }

            foundIdx2 = 1;
            variable_value2 = var->value[i];
            break;
        }
    }

    if(foundIdx2 == -1) {
        printf("Error: Variable with name '%s' not found\n",token.name);
        return (Expr){.type=TOKEN_ERR};
    }

    obj.valuevar2 = variable_value2;
    token = getNextToken(input);

    if(token.type != TOKEN_EOF_T) {
        printf("Error: Invalid arguments count passed\n");
        return (Expr){.type=TOKEN_ERR};
    }

    return obj;
}

void show_file_content(char *filename) {
    FILE *file = fopen(filename,"r");
    if(!file) {
        printf("Error: Failed to open this file\n");
        return;
    }

    char ch;
    while((ch = getc(file))!=EOF) 
        printf("%c",ch);
    fclose(file);
}

void help_command() {
    printf("\nCOMMANDS:\n");
    printf("  set <name>=<value        | creates variables\n");
    printf("  echo <string>            | prints a string\n");
    printf("  echo $<variable>         | prints the variable and his value\n");
    printf("  calc <var1> <op> <var2>  | calculates the operation with the variables value\n");
    printf("  create <filename>        | creates a file\n");
    printf("  rm <filename>            | deletes a file\n");
    printf("  cd <directory>           | goes to a different directory\n");
    printf("  cp <file1> <file2>       | copies the content of the first file to the second\n");
    printf("  ls                       | prints directorys\n");
    printf("  cls                      | clears the terminal screen\n");
    printf("  info                     | prints systems information\n");
    printf("  pwd                      | prints current working directory\n");
    printf("  show                     | prints all the stored variables\n");
    printf("  whoami                   | shows users shell name\n");
    printf("  cat <file_name>          | shows files content\n");
    printf("  delete <varname>         | deletes a variable\n");
    printf("  help                     | shows this pannel\n");
    printf("  exit                     | closes the program\n\n");
}

void print_neofetch() {
    printf("\n");
    printf("  ███╗   ██╗ █████╗  ██████╗ ███████╗     | CPU: Intel i8 Chip      | OS: Linux 6.5\n");
    printf("  ████╗  ██║██╔══██╗██╔════╝ ██╔════╝     | GPU: RTX 4090           | Kernel: x86_64\n");
    printf("  ██╔██╗ ██║███████║██║  ███╗█████╗       | SSD: 1TB                | Shell: bash\n");
    printf("  ██║╚██╗██║██╔══██║██║   ██║██╔══╝       | RAM: 64GB               | DE: GNOME 45\n");
    printf("  ██║ ╚████║██║  ██║╚██████╔╝███████╗     | NAME: Hacked_Linux      | Uptime: 12h 34m\n");
    printf("  ╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝     | Host: Custom-PC         | Packages: 432\n");
    printf("\n");
}

int main(void) {
    Variable var = {.counter = 0};
    char input[100];
    char command[50];
    Expr expr;

    while(1) {
        printf(">> ");
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
                    printf("Error: Division by 0!\n");
                    continue;
                } else {
                    printf("%.2f\n",expr.valuevar1/expr.valuevar2);
                }
            }
        } else if(strcmp(command, "echo") == 0) {
            char *ptr = input;
            getNextToken(&ptr);

            Token token = getNextToken(&ptr);
            if(token.type == TOKEN_DOLLAR) {
                if(var.counter == 0) {
                    printf("Error: List is empty\n");
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
                    printf("Error: Variable with the name '%s' not found\n",token.name);
                    continue;
                }

                token = getNextToken(&ptr);
                if(token.type != TOKEN_EOF_T) {
                    printf("Error: Invalid arguments count passed\n");
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
            } else {
                printf("Error: '$' is missing\n");
                continue;
            }
        } else if(strcmp(input, "show") == 0) {
            if(var.counter == 0) {
                printf("Error: List is empty\n");
                continue;
            }

            for(int i = 0; i < var.counter; i++) {
                if(var.type[i] == TOKEN_VALUE) {
                    printf("%s = %.2f\n", var.name[i], var.value[i]);
                } else {
                    printf("%s = \"%s\"\n",var.name[i], var.stringValue[i]);
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
                printf("Error: Invalid arguments count passed\n");
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

            if(counter == 2) {
                char prompt[100] = "touch ";
                strcat(prompt,tokens[1]);
                system(prompt);
            } else {
                printf("Error: Invalid arguments count passed\n");
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
                    printf("Error: File not found\n");
                    continue;
                }

                printf("Filed deleted succesfully\n");
                continue;
            } else {
                printf("Error: Invalid arguments count passed\n");
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
                    printf("Error: Invalid directory '%s'\n",tokens[1]);
                    continue;
                }
            } else {
                printf("Error: Invalid arguments count passed\n");
                continue;
            }
        } else if(strcmp(input,"pwd")==0) {
            long size = pathconf(".", _PC_PATH_MAX);
            if(size == -1) size = 4096;

            char *buff = malloc((size_t)size);
            if(buff == NULL) {
                printf("Error: Could not allocate buffer\n");
                continue;
            }

            if(getcwd(buff, (size_t)size) != NULL) {
                printf("%s\n", buff);
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
                    printf("Error: Failed to open source file '%s'\n", tokens[1]);
                    continue;
                }

                FILE *file2 = fopen(tokens[2], "w");
                if(file2 == NULL) {
                    printf("Error: Failed to create destination file '%s'\n", tokens[2]);
                    fclose(file1);
                    continue;
                }

                int ch;
                while((ch = fgetc(file1)) != EOF) 
                    fputc(ch, file2);
                

                fclose(file1);
                fclose(file2);
            } else {
                printf("Error: Invalid arguments count passed\n");
                continue;
            }
        } else if(strcmp(command,"delete")==0) {
            char *ptr = input;
            delete_variable(&var,&ptr);
        } else if(strcmp(input,"whoami")==0) {
            printf("USER: %s | PASSWORD: %d\n",USER_NAME,PASSWORD);
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
            printf("Error: Invalid command '%s'\n", input);
            continue;
        }
    }

    return 0;
}
