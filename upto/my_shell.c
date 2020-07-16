#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>

typedef char *char_ptr;

void parse(char_ptr line,char_ptr *argv) {
  while(*line != '\0') {
    while(*line == ' ') {
      *line++ = '\0';
    }
    *argv++ = line;
    while(*line != '\0' && *line != ' ') {
      line++;
    }
  }
  *argv = '\0';
};

int main(void) {
  char command[255];
  char_ptr argv[64];
  signal(SIGINT,SIG_IGN);
  while(1){
    printf("my-shell $ ");
    gets(command);
    parse(command,argv);
    int pid = fork();
    if(pid == 0) {
      signal(SIGINT,NULL);
      execvp(*argv,argv);
      printf("Command Not Found! %s \n", command);
      exit(0);
    }
    else {
      wait(&pid);
    }
  }
  return 0;
}