#include <stdio.h>

int main(int argc, char* argv[]) {
    char a = 0b10000000;
    printf("0X%X\n", a >> 7);
    return 0;
}