#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>

int main(void){
  int pid;
  pid = fork();
  if(pid == 0) {
    //child process
    int a;
    scanf("%d",&a);
  }
  else {
    // parent process
    printf("waiting in parent\n");
    wait(&pid);
    printf("finished waiting for child\n");
  }
  return 0;
}