#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#define swap(a,b) { a=a^b; b=a^b; a=b^a; }
int main() {
char s[] = "abcdefghijklmnopqrstuvwxyz";
printf("Original String : ");
puts(s);
unsigned i, c;
srand(time(NULL));
for (i = strlen(s) - 1; i > 0; --i) {
c = rand() % i;
swap(s[c], s[i]);
}
printf("\nSubstitute String : ");
puts(s);
}

