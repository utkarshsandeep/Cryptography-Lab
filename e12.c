#include <time.h>
#include <stdio.h>
#include <stdlib.h>
 
int main()
{
	char s[10],t[10];
    printf("Username : ");
	gets(s);
	printf("password : ");
	int p,r;
	scanf("%d",&p);
	printf("Check Credentials: ");
	printf("Enter username: ");
	gets(t);
	printf("Enter password : ");
	scanf("%d",&r);
	if(p==r)
	{
		printf("Hello ");
		puts(s);
	}
	else
	{
		printf("Password Incorrect! ");
	}
}
