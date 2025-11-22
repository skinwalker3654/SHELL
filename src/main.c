#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

typedef struct File_Write {
    char line[200][256];
    int counter;
} File_Write;

int main(void) {
    Variable var = {.counter = 0};
    char input[100];
    char command[50];
    Expr expr;

    char *buffer = find_pwd();
    if(buffer == NULL) return 1;

    File_Write file_ = {.counter=0};
    load_from_file(buffer,&var);

    while(1) {
        long size = pathconf(".",_PC_PATH_MAX);
        char buff[100];
        getcwd(buff,(size_t)size);

        printf(BOLD CYAN"%s: "RESET,USERNAME);
        printf(BLUE"%s "RESET,buff);
        printf(GREEN">> "RESET);
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
                    printf("%s\n",var.stringValue[indexIdx]);
                    continue;
                } else {
                    printf("%.2f\n",var.value[indexIdx]);
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
                printf(GREEN"File has been copied succesfully to '%s'\n"RESET,tokens[2]);
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
            Token token = getNextToken(&ptr);

            token = getNextToken(&ptr);
            if(token.type != TOKEN_NAME) {
                printf(RED"Error: Invalid FileName '%s'\n"RESET,token.name);
                continue;
            }

            char fileName[100];
            strcpy(fileName,token.name);

            token = getNextToken(&ptr);
            if(token.type != TOKEN_STRING) {
                printf(RED"Error: EndPoint must be inside of \"\"\n"RESET);
                continue;
            }

            char endPoint[100];
            strcpy(endPoint,token.stringValue);

            token = getNextToken(&ptr);
            if(token.type != TOKEN_EOF) {
                printf(RED"Error: Invalid arguments passed\n"RESET);
                continue;
            }

            FILE *fileTest = fopen(fileName,"r");
            if(fileTest == NULL) {
                printf(RED"Error: File '%s' does not exists\n"RESET,fileName);
                continue;
            }

            file_.counter = 0;
            char lineCheck[256];
            while(fgets(lineCheck,sizeof(lineCheck),fileTest)) {
                lineCheck[strcspn(lineCheck,"\n")] = 0;
                strcpy(file_.line[file_.counter],lineCheck); 
                file_.counter++;
            }

            fclose(fileTest);
            FILE *file = fopen(fileName,"w");
            if(file == NULL) {
                printf(RED"Error: Failed to open the file\n"RESET);
                continue;
            }

            if(strcmp(endPoint,"/show")==0) {
                printf(RED"Error: Invalid endPoint '%s' you cannot add this keyword as an endPoint\n",endPoint);
                continue;
            }

            char line[100];
            while(1) {
                printf("> ");
                fgets(line,sizeof(line),stdin);
                line[strcspn(line,"\n")] = 0;

                int number;
                if(sscanf(line,"/up %d",&number)==1) {
                    if(number < 1 || number > file_.counter+1) {
                        printf(RED"Error: Invalid size of number %d\n"RESET,number);
                        continue;
                    }

                    printf("%s : change this to > ",file_.line[number-1]);
                    fgets(line,sizeof(line),stdin);
                    line[strcspn(line,"\n")] = 0;

                    strcpy(file_.line[number-1],line);
                    continue;
                }

                if(strcmp(line,"/show")==0) {
                    for(int i=0; i<file_.counter; i++) 
                        printf("%d. %s\n",i+1,file_.line[i]);
                    continue;
                }

                int number2;
                if(sscanf(line,"/del %d",&number2)==1) {
                    for(int i=number2-1; i<file_.counter; i++) 
                        strcpy(file_.line[i],file_.line[i+1]);
                    file_.counter--;
                    continue;
                }

                if(strcmp(line,endPoint)==0) break;

                strcpy(file_.line[file_.counter],line);
                file_.counter++;
            }

            for(int i=0; i<file_.counter; i++)
                fprintf(file,"%s\n",file_.line[i]);

            fclose(file);
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
            char *ptr = input;
            Token token = getNextToken(&ptr);
            token = getNextToken(&ptr);
            if(token.type != TOKEN_STRING) {
                printf(RED"Error: Command must be inside of \"\"\n"RESET);
                continue;
            }

            char buff[100];
            strcpy(buff,token.stringValue);

            token = getNextToken(&ptr);
            if(token.type != TOKEN_EOF) {
                printf(RED"Error: Invalid arguments passed\n"RESET);
                continue;
            }
            
            system(buff);
        } else if(strcmp(command,"delete")==0) {
            char *ptr = input;
            delete_variable(&var,&ptr);
        } else if(strcmp(input,"whoami")==0) {
            printf(BLUE"USERNAME: "RESET CYAN"%s"RESET " | " BLUE "PASSWORD: "RESET CYAN"%d\n"RESET,USERNAME,PASSWORD);
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
        } else if(strcmp(command,"load")==0) {
            char *tokens[10];
            int counter = 0;

            char *token = strtok(input," ");
            while(token != NULL) {
                tokens[counter++] = token;
                token = strtok(NULL," ");
            }

            if(counter == 2)  {
                if(load_from_file(tokens[1],&var)==-1) {
                    printf(RED"Error: File '%s' does not exists\n"RESET,tokens[1]);
                    continue;
                }
            } else {
                printf(RED"Error: Invalid arguments passed\n"RESET); 
                continue;
            }
        } else if(strcmp(input,"reset")==0) {
            var.counter=0; 
            printf(GREEN"Data has been reseted succesfully\n"RESET);
        } else if(strcmp(input,"CALC HELP")==0) {
            print_calc_command();
        } else if(strcmp(input, "exit") == 0) {
            save_to_file(buffer,&var);
            free(buffer);
            printf(MAGENTA"Exiting...\n"RESET);
            return 0;
        } else {
            printf(RED"Error: Invalid command '%s'\n"RESET, input);
            printf(RED"Type: 'help' for more details\n"RESET);
            continue;
        }
    }

    return 0;
}
