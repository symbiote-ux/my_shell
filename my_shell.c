#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include <errno.h>
#include <fcntl.h>
#include<signal.h>
#include<stdlib.h>
#include <ctype.h>
#include<string.h>

typedef char *char_ptr;

int parse_pipe(char *input, char **commands, int pipe_count, int inputLen){
  int length = strlen(input);
  int command_value = 0;
  int idx = 0;
  for (int i = 0; i < length; i++)
  {
    if( input[i] !=  '|'){
      commands[command_value][idx] = input[i];
      idx++;
    }    
    else {
      commands[command_value][idx] = '\0';
      idx = 0;
      command_value++;
    }
  }
  return 0;
}
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
void print_cwd()
{
	char username[20];
	char cwd[100];
	gethostname(username, sizeof(username));
	getcwd(cwd, sizeof(cwd));
	printf("\033[0;32m%s:\033[0;35m%s ", username, cwd);
	printf("\033[1;0m");
}
void handle_cd(char_ptr *argv) {
  if(argv[1] == NULL) {
  	char *home = getenv("HOME");
  	if(chdir(home) != 0){
       perror("\033[0;31m chdir error");
  	}
     }else{
  	  if(chdir(argv[1]) != 0){
  		  perror("\033[0;31m chdir error");
  	}
  }
};
int count_pipe(char *input, int len){
  int count = 0;
  for(int i = 0; i <len; i++){
    if( input[i] == '|'){
      count+= 1;
    }
  }
  return count;
};
int is_redirection(char *command) {
  char *out = strstr(command,">");
  if(out != NULL) return 1;
  return -1;
};
int whitespaceCount(char *in, int len) {
  int i = 0;
  int count = 0; 
  for(i = 0; i < len; i++){
    if(in[i] == ' '){
      count++;
    }
  }
  return count;
}
void trim(char *str) {
  int i;
  int begin = 0;
  int end = strlen(str) - 1;
  while (isspace((unsigned char) str[begin])) begin++;
  while ((end >= begin) && isspace((unsigned char) str[end])) end--;
  for (i = begin; i <= end; i++) {
    str[i - begin] = str[i];
  }
  str[i - begin] = '\0';
}
int do_redirection(char *input) {
  char cpy[1000], path[1000], command[1000], filename[1000], **args, *temp;
  int spacecount, i = 0, fd;
  sprintf(cpy, "%s", input);
  temp = strtok(cpy, " ");
  sprintf(path, "%s", temp);
  sprintf(cpy, "%s", input);
  temp = strtok(cpy, ">");
  sprintf(command, "%s", temp);
  command[strlen(command) - 1] = '\0';
  temp = strtok(NULL, "\0");
  sprintf(filename, "%s", temp);
  trim(filename);
  spacecount = whitespaceCount(command, strlen(command));
  args = calloc((spacecount)+2, sizeof(char *));
  for(i = 0; i < spacecount + 1; i ++){
    args[i] = calloc(strlen(command)+10, sizeof(char));
  }
  parse(command, args);
  if((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0){
    perror("open error");
    return -1;
  }
  dup2(fd, STDOUT_FILENO);
  close(fd);
  execvp(path, (char *const *)args);
  perror("execvp error");
  _exit(EXIT_FAILURE);
};

void do_pipe(char **in, int *pipecount, int j){ 
  char cpy[1000], path[1000], command[1000], filename[1000], **args1,**args2, *temp;
  int spacecount, fd, i =0;
  spacecount = whitespaceCount(in[0], strlen(in[0]));
  args1 = calloc((spacecount)+2, sizeof(char *));
  for(i = 0; i < spacecount + 1; i ++){
    args1[i] = calloc(strlen(in[0])+10, sizeof(char));
  }
  parse(in[0], args1);
  spacecount = whitespaceCount(in[1], strlen(in[1]));
  args2 = calloc((spacecount)+2, sizeof(char *));
  for(i = 0; i < spacecount + 1; i ++){
    args2[i] = calloc(strlen(in[1])+10, sizeof(char));
  }
  parse(in[1], args2);
  int pipefds[2];
  int pipe_read = 0;
  int pipe_write = 1;
	pipe(pipefds);
	int pid = fork();
  if (pid == 0)
	{
		close(pipefds[pipe_read]);
		dup2(pipefds[pipe_write], STDOUT_FILENO);
    execvp(*args1,args1);
	}
	close(pipefds[pipe_write]);
	dup2(pipefds[pipe_read], STDIN_FILENO);
	execvp(*args2,args2);
}

int main(void) {
  char command[255];
  char_ptr argv[64];
  signal(SIGINT,SIG_IGN);
  while(1){
    printf("\033[1;96m\e[1m my-shell $");
		print_cwd();
    gets(command);
    if(strcmp(command, "exit") == 0){
      return 0;
    }
    int pid = fork();
    if(pid == 0) {
      signal(SIGINT,NULL);
      int num_of_pipes = count_pipe(command,64);
      if(num_of_pipes == 0) {
        int state;
        if( is_redirection(command) > 0) {
        do_redirection(command);
        continue;
        }
        parse(command,argv);
        if(strcmp(argv[0], "cd") == 0){
          handle_cd(argv);
        	continue;
        }
         execvp(*argv,argv);
        printf("\033[0;31mCommand Not Found!\033[1;31m %s \n", command);
        exit(0);
      }
      char **commands =  calloc(num_of_pipes +1, sizeof(char*));
      for(int i = 0; i < (num_of_pipes +1); i++){
        commands[i] = calloc((65), sizeof(char));
      }
      parse_pipe(command,commands,num_of_pipes, 10);
      for(int i = 0; i < (num_of_pipes + 1); i++){
        trim(commands[i]);
      }
      int count = num_of_pipes+1;
      do_pipe(commands, &count, 0);
      }
    else {
      wait(&pid);
    }
  }
}