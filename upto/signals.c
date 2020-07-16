#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<unistd.h>

int main(void){
  signal(SIGINT,SIG_IGN);
  int a = 0;
  while(1) {
    printf("%d\n",a);
    a++;
    sleep(1);
  }
  return 0;
}