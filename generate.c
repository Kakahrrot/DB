#include<stdio.h>

int main(void)
{
	FILE* fp = fopen("test.input", "w");
	unsigned long long int a = 1;
//	a <<= 60;
	a = 10000;
	printf("a = %llu\n", a);
	for(unsigned long long int i = 0; i < a; i++)
	{
		fprintf(fp, "PUT %llu %llu\n", i, i);
	}
}
