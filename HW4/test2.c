#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

int main(){
    int a = 10;
    char buf[1024];
    sprintf(buf, "a = %d\n", a);
    printf("%s\n", buf);
    return 0;
}