#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <sys/wait.h>
#include <libgen.h>
#include <linux/limits.h>
#include <errno.h>
#include <stdbool.h>

void printcwd(){
  char cwd[PATH_MAX];
  char *base;
  if (NULL == getcwd(cwd, sizeof(cwd))){
     fprintf(stderr,"Error: getcwd() failed");
  }
  else{
    base=basename(cwd);
  }
  printf("[%s]$ ", base);
  fflush(stdout);
}

char* readinput(){
  char* line;
  size_t size= 0;
  if (getline(&line, &size, stdin)==-1){
    if (feof(stdin)){
      exit(0);
    }
    else{
      fprintf(stderr, "Error: invalid command\n");
       printf("exiting read 2");
      exit(0);
    }
  }
  return line;
}

int argslength=0;

char** parseinput(char* line){
  int size= 64;
  char **tokens= malloc(size * sizeof(char*));
  char *token;
  if(!tokens){
    fprintf(stderr, "Error: invalid command\n");
    exit(0);
  }
  token=strtok_r(line, " \t\r\n", &line);
  while(token!=NULL){
    tokens[argslength]= token;
    argslength++;
    if(argslength>= size){
      size+=64;
      tokens= realloc(tokens, size*sizeof(char*));
      if (!tokens){
        exit(0);
      }
    }
    
    token= strtok_r(NULL, " \t\r\n", &line);
  }
  tokens[argslength]= NULL;
  return tokens;
}

int cd(char **args){
  if (args[1] == NULL || args[2] != NULL) {
    fprintf(stderr, "Error: invalid command\n");
  } 
  else {
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "Error: invalid directory\n");
    }
  }
  return 1;
}

int myexit(char** args){
  if (args[1]==NULL){
    return 0;
  }
  else{
    fprintf(stderr, "Error: invalid command\n");
    return 1;
  }
}

int IOredir(char** args, char* input, char* output, bool append){
  int fdin;
  int fdout;
  if (input !=NULL && output !=NULL){
    int stdout = dup(1);
    int stdin = dup(0);
    fdin = open(input, O_RDONLY); 
    if(fdin == -1){
        fprintf(stderr, "Error: invalid file\n"); 
        return 1; 
    }
    if (append){
      fdout = open(output,O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
      if(fdout == -1){ 
          fprintf(stderr, "Error: invalid file\n");
          return 1; 
      }
    }
    else{
      fdout = open(output,O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
      if(fdout == -1){ 
          fprintf(stderr, "Error: invalid file\n");
          return 1; 
      }
    }
    dup2(fdin, 0);
    dup2(fdout, 1);

    int status=1;
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr, "Error: fork failed");
    } 

    else if (pid == 0) {
      execvp(args[0], args);
      //exec failed
      fprintf(stderr, "Error: invalid program\n");
      exit(0);
    } 
    else {
      // parent
      do{
        waitpid(pid, &status, WUNTRACED);
      }while(!WIFEXITED(status) && !WIFSTOPPED(status));
    }
    dup2(stdin, 0); 
    dup2(stdout, 1); 
    close(stdin); 
    close(stdout); 
    close(fdin);
    close(fdout);
    return 1;
  }
  else if (input != NULL){
    int stdin = dup(0);
    fdin=open(input, O_RDONLY);
    if (fdin == -1){
      fprintf(stderr, "Error: invalid file\n");
      return 1;
    }
    if(dup2(fdin, 0)==-1){
      fprintf(stderr, "dup2");
      return 1;
    }
    int status=1;
    pid_t pid; 
    pid = fork(); 
    if (pid < 0) {
      fprintf(stderr, "Error: fork failed");
    } 
    else if (pid == 0) {
      execvp(args[0], args);
      fprintf(stderr, "Error: invalid program\n");
      exit(0);
    } 
    else {
        // parent
        do{
          waitpid(pid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSTOPPED(status));
    }
    dup2(stdin, 0); 
    close(stdin); 
    close(fdin);
    return 1;
  }
  
  else if (output !=NULL){
    int stdout = dup(1);
    if (append){
      fdout = open(output,O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
      if(fdout == -1){ 
          fprintf(stderr, "Error: invalid file\n");
          return 1; 
      }
    }
    else{
      fdout = open(output,O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
      if(fdout == -1){ 
          fprintf(stderr, "Error: invalid file\n");
          return 1; 
      }
    }
    if (dup2(fdout, 1)==-1){
      fprintf(stderr, "dup2 ouptut");
      return 1;
    }
    int status=1;
    pid_t pid; 
    pid = fork(); 
    if (pid < 0) {
      fprintf(stderr, "Error: fork failed");
    } 
    else if (pid == 0) {
      // child (new process)
      execvp(args[0], args);
      //exec failed
      fprintf(stderr, "Error: invalid program\n");
      exit(0);
    } 
    else {
      // parent
      do{
        waitpid(pid, &status, WUNTRACED);
      }while(!WIFEXITED(status) && !WIFSTOPPED(status));
    }
    dup2(stdout, 1); 
    close(stdout); 
    close(fdout);
    return 1;
  }
  else {
    fprintf(stderr, "Error: invalid file\n");
  }
  return 1;
}

int executecomm(char** args, char* input, char* output, bool append){
  if(args[0]==NULL){
     return 1;
  }
  char* builtin[] = {"cd","exit"};
  if (strcmp(args[0], builtin[0])==0){              //cd
    return cd(args);
  }
  else if(strcmp(args[0], builtin[1])==0){          //exit
    return myexit(args);
  }
  else{
    if (input != NULL || output !=NULL){
      return IOredir(args, input, output, append);
    }
    else{
      int status=1;
      pid_t pid = fork();
      if (pid < 0) {
        // fork failed (this shouldn't happen)
        fprintf(stderr, "Error: fork failed");
      } 
      else if (pid == 0) {
        // child (new process)
        execvp(args[0], args);
        fprintf(stderr, "Error: invalid program\n");
        exit(0);
      } 
      else {
        // parent
        do{
          waitpid(pid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSTOPPED(status));
      }
    }
  }
  return 1;
}

int parseIO(char** args){
  char* input= NULL;
  char* output= NULL;
  bool append= false;
  for (int i=1; i<argslength; i++){
    if(strcmp(args[i], "<") == 0){
      if(i<argslength-1){
        input=args[i+1];
        args[i]=NULL;
        i++;
      }
      else{
        fprintf(stderr, "Error: invalid command\n");
        return 1;
      }
    }
    else if(strcmp(args[i], ">") == 0){
      if(i<argslength-1){
        output=args[i+1];
         args[i]=NULL;
        i++;
      }
      else{
        fprintf(stderr, "Error: invalid command\n");
        return 1;
      }
    }
    else if(strcmp(args[i], ">>") == 0){
      if(i<argslength-1){
        output=args[i+1];
        args[i]=NULL;
        append= true;
        i++;
      }
      else{
        fprintf(stderr,"Error: invalid command\n");
        return 1;
      }
    }
    else if(strcmp(args[i], "<<") == 0){
      fprintf(stderr, "Error: invalid command\n");
      return 1;
    }
  }
  return executecomm(args, input, output, append);
}

int pipehandler(char** args, int pipecount){  
  if (args[0][0]==' '){
    fprintf(stderr, "Error: invalid command\n");
    return 1;
  }
  else{
    int in=0;
    int pipefd[2];
    int status=1;
    pid_t pid;
    pid_t pids[pipecount+1];
    int exitstats[pipecount+1];
    int numexited = 0;
    if (pipecount==1){
      if (pipe(pipefd) < 0) {
        fprintf(stderr, "pipe");
        return 1;
      }
      pid= fork();
      if (pid < 0) {
        fprintf(stderr, "Error: fork failed");
        return 1;
      }
      else if (pid == 0) {  // child process
          close(pipefd[0]);
          dup2(pipefd[1], 1);
          close(pipefd[1]);
          parseIO(parseinput(args[0]));
          exit(0);
      }
      else {
        pid_t pid2=fork();
        if (pid < 0) {
          fprintf(stderr, "Error: fork failed");
          return 1;
        } 
        else if (pid2==0){
          close(pipefd[1]);
          dup2(pipefd[0], 0);
          close(pipefd[0]);
          parseIO(parseinput(args[1]));
          exit(0);
        }
        else{
          close(pipefd[0]);
          close(pipefd[1]);
          waitpid(pid, &status, WUNTRACED);
          waitpid(pid2, &status, WUNTRACED);
        }
      }
      return 1;
    }
    else{
      for (int i=0; i<=pipecount;i++){
        if (pipe(pipefd) < 0) {
          fprintf(stderr, "pipe");
          break;
        }
        pid= fork();
        if (pid < 0) {
          fprintf(stderr, "Error: fork failed");
          return 1;
        }
        else if (pid == 0) {  // child process
          pids[i]=getpid();
          if (i==0){
            close(pipefd[0]);
            dup2(pipefd[1], 1);
            close(pipefd[1]);
            parseIO(parseinput(args[i]));
            exit(0);
          }
          else if (i != 0 && i!=pipecount) {
            dup2(in,0);
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            close(pipefd[1]);
            close(in);
            parseIO(parseinput(args[i]));
            exit(0);
          }
          else if (i == pipecount) {
            close(pipefd[1]);
            dup2(in, 0);
            close(pipefd[1]);
            close(in);
            parseIO(parseinput(args[i]));
            exit(0);
          }
        }
        else {  // parent process
          in=pipefd[0];
          close(pipefd[1]);
        }
      }
      for (int i=0; i<=pipecount;i++){ 
        close(pipefd[0]);
        close(in);
        waitpid(pids[i], &status, WUNTRACED);
        exitstats[i] = status;
        numexited++;
      }
      while (numexited <= pipecount) {
        for (int i=0; i<=pipecount; i++) {
          if (exitstats[i] == 0) {
            if (waitpid(pids[i], &status, WNOHANG) > 0) {
              exitstats[i] = status;
              numexited++;
            }
          }
        }
      }
    return 1;
    }
  }
}

int parsepipe(char* line){
  char *token; 
  int size=64;
  char **tokens = malloc(size * sizeof(char*));  
  int pipecount; 
  if (strchr(line, '|') != NULL) { 
    for (int i=0; i< (int) strlen(line); i++){
      if (line[i] == '|'){
        pipecount++;
      }
    } 
    int index = 0; 
    token = strtok_r(line, "|", &line);
    while (token!=NULL) {
      tokens[index] = token; 
      index++; 
      if(index>= size){
        size+=64;
        tokens= realloc(tokens, size*sizeof(char*));
      }
      token = strtok_r(line, "|", &line);
    }
    return pipehandler(tokens, pipecount); 
  } 
  else {
    return parseIO(parseinput(line));
  }
}

void handler(){}


int main(){
  signal(SIGINT, handler);
  signal(SIGQUIT, handler);
  signal(SIGTSTP, handler);
  char* input;
  int run=1;
   while (run){
    printcwd();
    input= readinput();
    run=parsepipe(input);

    free(input);
    argslength=0;
   }
}
