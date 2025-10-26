#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

int main(void) {
    Variable var = {.counter = 0};
    char input[100];
    char command[50];
    Expr expr;

    char *buffer = find_pwd();
    if(buffer == NULL) return 1;

    load_from_file(buffer,&var);
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
            if(expr.type == TOKEN_SQUARE) {
                if(expr.isName == 0) {
                    printf("Square root of %s = %.2f\n",expr.Squares.varName,sqrt(expr.Squares.square_value));
                    continue;
                } else if(expr.isName == 1) {
                    printf("Square root of %.2f = %.2f\n",expr.Squares.square_value,sqrt(expr.Squares.square_value));
                    continue;
                }
            }

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
                printf(GREEN"File created succesfully\n"RESET);
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
            long size = pathconf(".",_PC_PATH_MAX);
            if(size == -1) size = 4096;

            char *newBuff = malloc((size_t)size);
            if(!newBuff) {
                printf(RED"Error: Memory allocation failed\n"RESET);
                return 1;
            }
    
            getcwd(newBuff, (size_t)size);
            printf(BLUE"%s\n"RESET,newBuff);
            free(newBuff);
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
                printf(GREEN"File has been copied succesfully into '%s'\n"RESET,tokens[2]);
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

                fseek(file, 0, SEEK_END);
                long size = ftell(file);

                fclose(file);
                printf("Total bytes of \"%s\" -> %ld\n",tokens[1],size);
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
                printf(RED"Error: Text must be inside \"\"\n"RESET);
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
        } else if(strcmp(command,"run")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 2) {
                pid_t pid = fork();
                if(pid == -1) {
                    printf(RED"Error: Fork failed\n"RESET);
                    continue;
                } else if(pid == 0) {
                    execl(tokens[1],tokens[1],NULL);
                    printf(RED"Error: Invalid executable file\n"RESET);
                    _exit(127);
                } else {
                    int status;
                    waitpid(pid,&status,0);
                }
            } else {
                printf(RED"Error: Invalid arguments count passed\n"RESET);
                continue;
            }
        } else if(strcmp(command,"custom")==0) {
            char *content = input + strlen("custom");
            while(isspace(*content)) content++;

            if(strlen(content) == 0) {
                printf(RED"Error: No command passed\n"RESET);
                continue;
            }

            system(content);
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
            print_fetch();
        } else if(strcmp(input,"CALC HELP")==0) {
            print_calc_command();
        } else if(strcmp(input, "exit") == 0) {
            save_to_file(buffer,&var);
            free(buffer);
            printf(MAGENTA"Exiting...\n"RESET);
            return 0;
        } else {
            printf(RED"Error: Invalid command '%s'\n"RESET, input);
            continue;
        }
    }

    return 0;
}
