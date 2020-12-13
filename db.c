#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEGREE 2 // 3 4 tree
typedef struct treenode
{
	unsigned long long int *key;
	char** value;
	int t;
	struct treenode** c;
	int n; //current number of keys
	int size;//current number of key-value pair in the sub-tree
	int leaf;
}TN;

TN* creatNode(int t, int leaf)
{
	TN* nd = malloc(sizeof(TN));
	nd -> key = malloc(sizeof(unsigned long long int) * (2 * t - 1));
	nd -> value = malloc(sizeof(char*) * (2 * t - 1));
	for(int i = 0; i < 2 * t - 1; i++)
		nd->value[i] = malloc(sizeof(char) * 129);
	nd -> c = malloc(sizeof(TN*) * 2 * t);
	nd -> t = t;
	nd -> leaf = leaf;
	nd -> n = 0;
	nd -> size = 0;
	return nd;
}

void traverse(TN* root)
{
	if(root -> leaf)
	{
		for(int i = 0; i < root -> n; i++)
			printf("key: %llu, value: %s\n", root -> key[i], root -> value[i]);
		return;
	}
	else
	{
		for(int i = 0; i < root -> n; i++)
		{
			traverse(root -> c[i]);
			printf("key: %llu, value: %s\n", root -> key[i], root -> value[i]);
		}
		traverse(root -> c[root -> n]);
	}
}

char* search(TN* root, unsigned long long int key)
{
	int i = 0;
	while(root -> key[i] < key && i < root -> n)
		i++;
	if(root -> key[i] == key)
		return root -> value[i];
	if(root -> leaf)
		return NULL;
	return search(root -> c[i], key);
}

void splitChild(TN* root, int i, TN* y)
{
	TN* z = creatNode(DEGREE, y -> leaf);
	z -> n = DEGREE - 1;
	y -> n = DEGREE - 1;
	for(int j = 0; j < DEGREE - 1; j++)
	{
		z -> key[j] = y -> key[j + DEGREE];
		strcpy(z ->value[j], y->value[j + DEGREE]);
	}
	y -> size = DEGREE- 1;
	z -> size = DEGREE- 1;
	if(! y -> leaf)
	{
		for(int j = 0; j < DEGREE; j++)
			z -> c[j] = y -> c[j + DEGREE];
		// calculate size of sub tree
		for(int j = 0; j < DEGREE; j++)
		{
			y -> size += y -> c[j] -> size;
			z -> size += z -> c[j] -> size;
		}
	}
	
	for(int j = root -> n; j >= i+1; j--)
		root -> c[j + 1] = root -> c[j];
	root -> c[i + 1] = z;
	for(int j = root -> n - 1; j >= i; j--)
	{
		root -> key[j + 1] = root ->key[j];
		strcpy(root -> value[j + 1], root -> value[j]);
	}
	root -> key[i] = y -> key[DEGREE - 1];
	strcpy(root -> value[i], y -> value[DEGREE - 1]);
	root -> n++;
}

int insertNonFull(TN* root, unsigned long long int key, char* value)
{
	int i = root -> n - 1;
	if(root -> leaf)
	{
		while(i >= 0 && root -> key[i] > key)
			i--;
		if(root -> key[i] == key) // find the key and update the value
		{
			strcpy(root -> value[i], value);
			return 0;
		}
		i = root -> n - 1;
		while(i >= 0 && root -> key[i] > key)
		{
			root -> key[i + 1] = root -> key[i];
			strcpy(root -> value[i + 1], root -> value[i]);
			i--;
		}
		root -> key[i + 1] = key;
		strcpy(root -> value[i + 1], value);
		root -> n++;
		return 1;
	}
	else
	{
		while(i >= 0 && root -> key[i] > key)
			i--;
		if(root -> key[i] == key) // find the key and update the value
		{
			strcpy(root -> value[i], value);
			return 0;
		}
		if(root -> c[i + 1] -> n == 2 * DEGREE - 1)		
		{
			//printf("before\n");
			//traverse(root);
			//printf("size = %d\n", (root) -> size);
			splitChild(root, i + 1, root -> c[i + 1]);
			if(root -> key[i + 1] == key) // find the key and update the value
			{
				strcpy(root -> value[i + 1], value);
				return 0;
			}
			if(root -> key[i + 1] < key)
				i++;
		}
		int tmp = insertNonFull(root -> c[i + 1], key, value);
		root -> c[i + 1] -> size += tmp;
		return tmp;
	}
}

void insert(TN** root, unsigned long long int key, char* value)
{
	if(! *root)
	{
		*root = creatNode(DEGREE, 1);
		(*root) -> key[0] = key;
		strcpy((*root) -> value[0], value);
		(*root) -> n  = 1;
		(*root) -> size++;
	}
	else
	{
		if((*root) -> n == 2 * (*root) -> t - 1)
		{
			TN* tmp = creatNode(DEGREE, 0);
			tmp -> size = (*root) -> size; 
			tmp -> c[0] = *root;
			splitChild(tmp, 0, *root);
			//int i = 0;
			//if(tmp ->key[0] < key)
			//	i++;
			//tmp -> size += insertNonFull(tmp -> c[i], key, value);
			tmp -> size += insertNonFull(tmp, key, value);
			*root = tmp;
		}
		else
			(*root) -> size += insertNonFull(*root, key, value);
	}
}


int main(int argc, char* argv[])
{
	TN* rt = NULL;
	FILE* fp = fopen(argv[1], "r");
	char* p1 = strtok(argv[1], "./");
	char* p2 = NULL;
	while(1)
	{
		p2 = strtok(NULL, "./");
		if(strcmp(p2, "input") == 0)
		{
			//printf("p1 = %s\n", p1);
			p2 = p1;
			break;
		}
		p1 = strtok(NULL, "./");
		if(strcmp(p1, "input") == 0)
		{
			//printf("p2 = %s\n", p2);
			p1 = p2;
			break;
		}
	}
	//printf("%s %s %ld\n", p1, p2, strlen(p1));
	p1 = (char*) malloc(sizeof(char) * (strlen(p2) + 8));
	strcat(p1, p2);
	strcat(p1, ".output");
	FILE* fp_out = fopen(p1, "w");
	char command[4];
	unsigned long long int key, key2;
	char value[129];
	int counter = 0;
	while(fscanf(fp, "%s", command) != EOF)
	{
		printf("%s %d\n", command, counter++);
		if(strcmp(command, "PUT") == 0)
		{
			fscanf(fp, "%llu %s", &key, value);
			insert(&rt, key, value);
		}
		else if(strcmp(command, "GET") == 0)
		{
			fscanf(fp, "%llu", &key);
			char* s = search(rt, key);
			if(s)
				fprintf(fp_out, "%s\n", s);
			else
				fprintf(fp_out, "EMPTY\n");
		}
		else if(strcmp(command, "SCAN") == 0)
		{
			fscanf(fp, "%llu %llu", &key, &key2);
			for(unsigned long long int i = key; i <= key2; i++)
			{
				char* s = search(rt, i);
				if(s)
					fprintf(fp_out, "%s\n", s);
				else
					fprintf(fp_out, "EMPTY\n");
			}
		}
	}
	//traverse(rt);
	//printf("size = %d\n", rt -> size);

}
