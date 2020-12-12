#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DEGREE 2
typedef struct treenode
{
	unsigned long long int *key;
	char** value;
	int t;
	struct treenode** c;
	int n; //current number of keys
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
	for(int j = 0; j < DEGREE - 1; j++)
	{
		z -> key[j] = y -> key[j + DEGREE];
		strcpy(z ->value[j], y->value[j + DEGREE]);
	}
	if(! y -> leaf)
		for(int j = 0; j < DEGREE; j++)
			z -> c[j] = y -> c[j + DEGREE];
	y -> n = DEGREE - 1;
	for(int j = root -> n; j >= i+1; j--)
		root -> c[j + 1] = root -> c[j];
	root -> c[i + 1] = z;
	for(int j = root -> n - 1; j >= i; j--)
	{
		root -> key[j + 1] = root ->key[j];
		strcpy(root -> value[j + 1], root -> value[j]);
	}
	root ->key[i] = y -> key[DEGREE - 1];
	strcpy(root -> value[i], y -> value[DEGREE - 1]);
	root -> n ++;
}

void insertNonFull(TN* root, unsigned long long int key, char* value)
{
	int i= root -> n - 1;
	if(root -> leaf)
	{
		while(i >= 0 && root -> key[i] > key)
			i--;
		if(root -> key[i] == key) // find the key and update the value
		{
			strcpy(root -> value[i], value);
			return;
		}
		i= root -> n - 1;
		while(i >= 0 && root -> key[i] > key)
		{
			root -> key[i + 1] = root -> key[i];
			strcpy(root -> value[i + 1], root -> value[i]);
			i--;
		}
		root -> key[i + 1] = key;
		strcpy(root -> value[i + 1], value);
		root -> n++;
	}
	else
	{
		while(i >= 0 && root -> key[i] > key)
			i--;
		if(root -> key[i] == key) // find the key and update the value
		{
			strcpy(root -> value[i], value);
			return;
		}
		if(root -> c[i + 1] -> n == 2 * DEGREE - 1)		
		{
			splitChild(root, i + 1, root -> c[i + 1]);
			if(root -> key[i + 1] == key) // find the key and update the value
			{
				strcpy(root -> value[i + 1], value);
				return;
			}
			if(root -> key[i + 1] < key)
				i++;
		}
		insertNonFull(root -> c[i + 1], key, value);
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
	}
	else
	{
		if((*root) -> n == 2 * (*root) -> t - 1)
		{
			TN* tmp = creatNode(DEGREE, 0);
			tmp -> c[0] = *root;
			splitChild(tmp, 0, *root);
			int i = 0;
			if(tmp ->key[0] < key)
				i++;
			insertNonFull(tmp -> c[i], key, value);
			*root = tmp;
		}
		else
			insertNonFull(*root, key, value);
	}
}


int main()
{
	TN* rt = NULL;
	insert(&rt, 1, "hi");
	insert(&rt, 2, ", ");
	insert(&rt, 4, "name ");
	insert(&rt, 3, "my ");
	insert(&rt, 5, "is ");
	insert(&rt, 1, "HI");
	traverse(rt);
}
