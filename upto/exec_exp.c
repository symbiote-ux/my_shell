#include <stdio.h>
#include <unistd.h>

int main(void) {
  int a;
  printf("Enter a number\n");
  scanf("%d",&a);
  int x = execl("/bin/cat","/bin/cat",(char *)NULL);
  printf("finish running exec");
}