#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

int main(void) {
  char command[255];
  while(1){
    printf("my-shell $\n");
    gets(command);
    execl("bin/echo","bin/echo","hello",NULL);
  }
  return 0;
}