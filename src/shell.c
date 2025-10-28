#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "shell.h"

Token getNextToken(char **input) {
    while(isspace(**input)) (*input)++;

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
        } else if(strcmp(token.name,"custom")==0) {
            token.type = TOKEN_CUSTOM;
            return token;
        } else if(strcmp(token.name,"square")==0) {
            token.type = TOKEN_SQUARE;
            return token;
        } else if(strcmp(token.name,"write")==0) {
            token.type = TOKEN_WRITE;
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
        while(**input != '\0' && !(**input == '"' && *(*input-1)!='\\')) {
            if(**input == '\\' && *(*input+1) != '\0') {
                (*input)++;
                token.stringValue[counter] = **input;
                counter++;
            } else {
                token.stringValue[counter] = **input;
                counter++;
            }
            (*input)++;
        }

        token.stringValue[counter] = '\0';
        if(**input == '"') (*input)++;
        else { 
            printf(RED"Error: Forgot to close the parenthesis -> \"\n"RESET);
            return (Token){TOKEN_ERR};
        }

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


void save_to_file(char *fileNname,Variable *ptr) {
    FILE *file = fopen(fileNname,"w");
    if(!file) {
        printf("Error: Failed to open the file\n");
        return;
    }

    for(int i=0; i<ptr->counter; i++) {
        if(ptr->type[i] == TOKEN_VALUE) {
            fprintf(file,"set %s=%f\n",ptr->name[i],ptr->value[i]);
        } else {
            fprintf(file,"set %s=\"%s\"\n",ptr->name[i],ptr->stringValue[i]);
        }
    }

    fclose(file);
}

void parseTokens(char **input, Variable *var) {
    Token token = getNextToken(input);
    int idx = var->counter;

    if(token.type != TOKEN_SET) {
        printf(RED"Error: 'set' is missing\n"RESET);
        return;
    }

    token = getNextToken(input);
    if(token.type != TOKEN_NAME) {
        printf(RED"Error: Invalid variable name '%s'\n"RESET,token.name);
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
        printf(RED"Error: This variable name already exists '%s'\n"RESET,token.name);
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
        printf(RED"Error: Invalid value on variable '%s'\n"RESET,token.name);
        return;
    } else {
        if(token.type == TOKEN_VALUE) {
            var->value[idx] = token.value;
            var->type[idx] = TOKEN_VALUE;
        } else if(token.type == TOKEN_STRING) {
            strcpy(var->stringValue[idx],token.stringValue);
            var->type[idx] = TOKEN_STRING;
        } else {
            printf(RED"Error: Strings must have \"\"\n"RESET);
            return;
        }
    }

    token = getNextToken(input);
    if(token.type != TOKEN_EOF) {
        printf(RED"Error: Invalid arguments passed\n"RESET);
        return;
    }

    var->counter++;
}

int load_from_file(char *fileName,Variable *ptr) {
    FILE *file = fopen(fileName,"r");
    if(!file) { return -1; }

    char line[100];
    while(fgets(line,sizeof(line),file)) {
        char *ptrString = line;
        parseTokens(&ptrString,ptr);
    }

    fclose(file);
    return 0;
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

    for(int i=index; i<var->counter-1; i++) {
        strcpy(var->name[i],var->name[i+1]);
        var->value[i] = var->value[i+1];
        strcpy(var->stringValue[i],var->stringValue[i+1]);
        var->type[i] = var->type[i+1];
    }

    var->counter--;
    printf(GREEN"Variable '%s' deleted succesfully\n"RESET,token.name);
}

int parseSquare(char **input,Variable *var,Token token,Expr *obj) {
    token = getNextToken(input);
    if(token.type != TOKEN_VALUE && token.type != TOKEN_NAME) {
        printf(RED"Error: Invalid token '%s'\n"RESET,token.name);
        return -1;
    }

    if(token.type == TOKEN_VALUE) {
        obj->isName = 1;
        obj->Squares.square_value = token.value;
    } else {
        int foundIdx = -1;
        float variable_value;
        for(int i=0; i<var->counter; i++) {
            if(strcmp(token.name,var->name[i])==0) {
                if(var->type[i] == TOKEN_STRING) {
                    printf(RED"Error: Cannot do operations with strings\n"RESET);
                    return -1;
                }

                foundIdx = 1;
                variable_value = var->value[i];
                break;
            }
        }

        if(foundIdx == -1) {
            printf(RED"Error: Variable with name '%s' not found\n"RESET,token.name);
            return -1;
        }
        
        obj->isName = 0;
        strcpy(obj->Squares.varName,token.name);
        obj->Squares.square_value = variable_value;
    }

    obj->type = TOKEN_SQUARE;
    return 0;
}

Expr parseExpr(char **input,Variable *var) {
    Expr obj;

    Token token = getNextToken(input);
    if(token.type != TOKEN_CALC) {
        printf(RED"Error: 'calc' is missing\n"RESET);
        return (Expr){.type=TOKEN_ERR};
    }

    token = getNextToken(input);
    if(token.type == TOKEN_SQUARE) {
        int check = parseSquare(input,var,token,&obj);
        if(check == -1) return (Expr){.type=TOKEN_ERR};
        else { return obj; }
    }

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

void show_file_content(char *fileName) {
    FILE *file = fopen(fileName,"r");
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
    printf(CYAN"  set <name>=<value>           |  creates variables\n"RESET);
    printf(CYAN"  echo <string>                |  prints a string\n"RESET);
    printf(CYAN"  echo $<variable>             |  prints the variable and his value\n"RESET);
    printf(CYAN"  CALC HELP                    |  This command prints the usages of 'calc' command\n"RESET);
    printf(CYAN"  create <fileName>            |  creates a file\n"RESET);
    printf(CYAN"  rm <fileName>                |  deletes a file\n"RESET);
    printf(CYAN"  cd <directory>               |  goes to a different directory\n"RESET);
    printf(CYAN"  cp <file1> <file2>           |  copies the content of the first file to the second\n"RESET);
    printf(CYAN"  fsize <filename>             |  it shows you the size of a file\n"RESET);
    printf(CYAN"  write <fileName> <endPoint>  |  writes text into a file till you type the endPoint\n"RESET);
    printf(CYAN"  run <executable>             |  it runs executable files\n"RESET);
    printf(CYAN"  custom <command>             |  it executes an already existing bash command\n"RESET);
    printf(CYAN"  load <fileName>              |  loads the filenames script\n"RESET);
    printf(CYAN"  countvars                    |  it shows you the total number of stored variables\n"RESET);
    printf(CYAN"  ls                           |  prints directorys\n"RESET);
    printf(CYAN"  cls                          |  clears the terminal screen\n"RESET);
    printf(CYAN"  info                         |  prints systems information\n"RESET);
    printf(CYAN"  show                         |  prints all the stored variables\n"RESET);
    printf(CYAN"  whoami                       |  shows users shell name\n"RESET);
    printf(CYAN"  reset                        |  deletes every variable in the list\n"RESET);
    printf(CYAN"  cat <fileName>               |  shows files content\n"RESET);
    printf(CYAN"  delete <varName>             |  deletes a variable\n"RESET);
    printf(CYAN"  help                         |  shows this pannel\n"RESET);
    printf(CYAN"  exit                         |  closes the program\n\n");
}

void print_calc_command() {
    printf(GREEN"\nCALC COMMAND: This command executes basic math operations with numbers or variables\n\n"RESET);
    printf("Usage: "); 
    printf(CYAN"calc <num/vaiable> <op> <num/variable>\n"RESET);
    printf("Available operations: ");
    printf(CYAN"'+' '-' '*' '/' '^'\n\n"RESET);
    printf(GREEN"You can also find square roots: "); 
    printf(CYAN"calc square <num/var>\n\n"RESET);
}

void print_fetch() {
    printf("\n");
    printf(RED   "  ███╗   ██╗ █████╗  ██████╗ ███████╗ " RESET YELLOW "    | CPU: Intel i8 Chip      | OS: Linux 6.5\n");
    printf(GREEN "  ████╗  ██║██╔══██╗██╔════╝ ██╔════╝ " RESET BLUE   "    | GPU: RTX 4090           | Kernel: x86_64\n");
    printf(CYAN  "  ██╔██╗ ██║███████║██║  ███╗█████╗   " RESET MAGENTA"    | SSD: 1TB                | Shell: bash\n");
    printf(WHITE "  ██║╚██╗██║██╔══██║██║   ██║██╔══╝   " RESET YELLOW "    | RAM: 64GB               | DE: GNOME 45\n");
    printf(RED   "  ██║ ╚████║██║  ██║╚██████╔╝███████╗ " RESET CYAN   "    | NAME: Hacked_Linux      | Uptime: 12h 34m\n");
    printf(GREEN "  ╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝ " RESET BLUE   "    | Host: Custom-PC         | Packages: 432\n");
    printf("\n");
}

char *find_pwd() {
    long size = pathconf(".",_PC_PATH_MAX);
    char *buff = malloc(100);
    if(!buff) return NULL;
    getcwd(buff, (size_t)size);
    strcat(buff,"/data.txt");
    return buff;
}
