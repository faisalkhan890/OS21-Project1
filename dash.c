

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include<fcntl.h>

    
    int dash_built_in_execution(int builtinCommandCode, char **commands,int i ,int commandLength);
    void dash_exit(char **args);
    int dash_help(char **args, int i, int commandLength);
    int dash_cd(char **args);
    int dash_path(char **args);
    int systemFuncExec (char **commands, char *nameOfFile, bool redirect);

    void executeCommand(char **commands,char *nameOfFile,bool redirect);
    char **commandSeparator(char *lineGiven,char *separatorArg);
    void errorFunc(char *errorMessage);
    void inputProcessFunc(char *lineGiven);


    //global variable, the initial system command searchning path
    char systemCommandPath[150]="/bin /usr/bin";

    //global variable, all build in function we have.
    char *builtInsNames[]={"exit","help","cd","path"};



    //separate the command by separator Arguement which is space in this case.
    char **commandSeparator(char *lineGiven,char *separatorArg)
    {
        int bufferSize=150;
    
        char **commands=malloc(bufferSize*sizeof(char *));
        char *comd=NULL;
        
        int i=0;
        comd=strtok(lineGiven,separatorArg);
        
        while(comd!=NULL)
        {
            commands[i]=comd;
            i++;
            comd=strtok(NULL,separatorArg);
        }
        commands[i]=NULL;
        return commands;
    }


    //show error message.
    void errorFunc(char *errorMessage)
    {
        if(strcmp(errorMessage, "")!=0)
        {
            perror(errorMessage);
        }
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }


    //execute the built in command and system command.
    void executeCommand(char **commands,char *nameOfFile,bool redirect)
    {
    //determine which built in command number is called
    int commandLength=sizeof(builtInsNames)/sizeof(builtInsNames[0]);
   
    //matching built in commands with their numbers
    int builtinCommandCode=-1;
    int i;
    for(i=0;i<commandLength;i++)
    {
        if(strcasecmp(builtInsNames[i],commands[0])==0)
        {
            builtinCommandCode=i;
            break;
        }
    }
    //built in functions execution
    if(builtinCommandCode!=-1) // checking for wrong input
    {
   dash_built_in_execution(builtinCommandCode, commands, i ,commandLength); //built in command execution.
    }
    
    
    //system function execution.
    else{
      systemFuncExec(commands, nameOfFile,  redirect);
    }
}


//process the input/batch command first.
//check if it is parallel or redirection command. then execute Command
void inputProcessFunc(char *lineGiven){
    char **redirectCommands=NULL;
    char **parallelCommands=NULL;
    char **commands=NULL;
    
    //Parallel check
    parallelCommands=commandSeparator(lineGiven, "&");
    
    int i=0;
    //if not parallel, run once. else run all parallel command.
    while(parallelCommands[i]!=NULL){
        commands=NULL;
        //Redirection check
        //check redirection command error
        char *redirectError=strstr(parallelCommands[i], ">>");
        if(redirectError){
            printf("%s","Error: Multiple redirection operators!\n");
            errorFunc("");
            break;
        }
        //check redirection command error
        redirectCommands=commandSeparator(parallelCommands[i], ">");
        if(redirectCommands[1]!=NULL && redirectCommands[2]!=NULL){
            printf("%s","Error: Multiple redirection operators!\n");
            errorFunc("");
            break;
        }
        //REdirection
        else if(redirectCommands[1]!=NULL){
            char **nameOfFile=commandSeparator(redirectCommands[1], " \t\r\n\a");
            //check redirection command error.
            if(nameOfFile[1]!=NULL){
                printf("%s","Error: Multiple redirection files!\n");
                errorFunc("");
                break;
            }
            commands=NULL;
            commands=commandSeparator(redirectCommands[0]," \t\r\n\a");
            if(commands[0]!=NULL){
                executeCommand(commands, nameOfFile[0], true);
            }
        }
        //NOT Redirection
        else{
            commands=NULL;
            commands=commandSeparator(redirectCommands[0]," \t\r\n\a");
            if(commands[0]!=NULL){
                executeCommand(commands, NULL, false);
            }
        }
        i++;
    }
}

int systemFuncExec(char **commands, char *nameOfFile, bool redirect)
{
  int status;
        pid_t pid = fork();

        // child process
        if (pid == 0) {
            //prepare the command  searching path.
            char **commandPaths=commandSeparator(systemCommandPath, " \t\r\n\a");
            bool PathFind=false;
            
            int i=0;
            char *cmdPath=NULL;
            while(commandPaths[i]!=NULL){
                cmdPath=(char *)malloc(strlen(commandPaths[i])+strlen(commands[0])+1);
                strcpy(cmdPath, commandPaths[i]);
                strcat(cmdPath, "/");
                strcat(cmdPath, commands[0]);
                
                //check access and execv
                if(access(cmdPath, X_OK)==0){
                    PathFind=true;
                    //if not redirection, then execute directly.
                    if(!redirect){
                        if(execv(cmdPath, commands)==-1){
                            errorFunc("dash error");
                            exit(EXIT_FAILURE);
                        }
                    }
                    break;
                }
                free(cmdPath);
                i++;
            }
            
            //if redirection, output result.
            if(redirect){
                int fd = open(nameOfFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);
                if(PathFind){
                    execv(cmdPath,commands);
                }
            }
            
            //error if cannot find command at any path.
            if(!PathFind){
                errorFunc("Invalid Command! Please check your input or searching path!");
                exit(EXIT_FAILURE);
            }
        }
        
        // fork error
        else if (pid < 0) {
            errorFunc("dash fork error");
        }
        
        // parent process, wait child to finish.
        else {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        return 1;
}

int dash_built_in_execution(int builtinCommandCode, char **commands, int i ,int commandLength)
{
 
        if (builtinCommandCode == 0) //build in command: exit.
        {
                dash_exit(commands);
        }
        else if(builtinCommandCode==1 ) //build in command: help.
            {
               dash_help(commands , i , commandLength);
            }
                
            else if (builtinCommandCode == 2)   //build in command: cd
                {
                  dash_cd(commands);
                }
             else if (builtinCommandCode == 3)  //build in command: path
               {
                dash_path(commands);
               }
     return 1;
}


void dash_exit(char **commands){
    if(commands[1]!=NULL){
                    errorFunc("exit cannot have any arguments!");
                }
                else{
                    exit(0);
                }
}

int dash_help(char **commands, int i, int commandLength)
{
  printf("%s","**** This is a help function ****\n");
                printf("%s","We provide below build in functions:\n");
                for(i=0;i<commandLength;i++) // printing builtin functions
                {
                    printf("%s ",builtInsNames[i]);
                }
                printf("\n");
                return 1;
}

int dash_cd(char **commands)
{
   if(commands[1]==NULL){
                    errorFunc("CD cannot find any argument!");
                }
                else if(commands[2]!=NULL){
                    errorFunc("CD command can only take one argument!");
                }
                else {
                    if(chdir(commands[1])!=0){
                        errorFunc("dash error");
                    }
                }
                return 1;
}

int dash_path(char **commands)
{
strcpy(systemCommandPath, "");
                int j=1;
                while(commands[j]!=NULL){
                    strcat(systemCommandPath, commands[j]);
                    strcat(systemCommandPath," ");
                    j++;
                }
                return 1;
}

int main(int argc, const char * argv[]) {
  
    //batch mode, get name Of File from argv[1]
    if(argv[1]!=NULL) //checking if there are any arguements or not
    {
        //if the shell is invoke more than one file.
        if(argv[2]!=NULL){
            errorFunc("Invoke more than one file.");
            exit(1);
        }
        char const* const nameOfFile=argv[1];
        FILE *file=fopen(nameOfFile, "r");
        char lineGiven[256];
        
        if(file==NULL){
            errorFunc("read file error!");
            exit(1);
        }
        
        while(fgets(lineGiven, sizeof(lineGiven), file)){
            //process command in batch file, line by line.
            inputProcessFunc(lineGiven);
        }
        
        fclose(file);
        exit(0);
    }
    
    
    //interactive mode, loop until exit command.
    while(1){
        printf("dash> ");
        
        char *lineGiven = NULL;
        size_t len=0;
        getline(&lineGiven,&len,stdin);
        
        //process input command.
        inputProcessFunc(lineGiven);
        
    }
    
    return 0;
}

//end of program
