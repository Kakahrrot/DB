#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	FILE* fp = fopen("test.input", "w");
	unsigned long long int a = 1;
	a = 180000000;
	printf("a = %llu\n", a);
	for(unsigned long long int i = 0; i < a; i++)
		fprintf(fp, "PUT %llu %llu\n", i, i);
	/*
	for(unsigned long long int i = 0; i < a; i++)
		fprintf(fp, "PUT %llu %llu\n", rand(), rand());
	for(unsigned long long int i = 0; i < 1024; i++)
		fprintf(fp, "GET %llu \n", rand());
	*/
}
