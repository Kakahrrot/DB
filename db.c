#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEGREE 1024 * 1024


typedef struct treenode
{
	unsigned long long int *key;
	char** value;
	int t;
	struct treenode** c;
	struct treenode* parent;
	int filenum;
	int* childNum;
	int n; //current number of keys
	int size;//current number of key-value pair in the sub-tree
	int access;
	int leaf;
}TN;

int counter = 0;
int filenumber = 1;
TN* ROOT = NULL;
int NODESIZE = sizeof(TN) + (sizeof(unsigned long long int) + sizeof(char*) + sizeof(char) * 129) * (2 * DEGREE - 1) + (sizeof(TN*) + sizeof(int)) * 2 * DEGREE;
int TREESIZE = 0;//((1024 * 1024 * 1024 / NODESIZE * 3) % 1000 ) * 1000; 

typedef struct st{
	TN* node;
	int onMem;
	struct st* next;
}stack;

TN* creatNode(int t, int leaf, TN* parent);

stack *st = NULL;
stack *end = NULL;
void push(TN* nd)
{
	if(!st)
	{
		st = malloc(sizeof(stack));
		st -> node = nd;
		st -> next = NULL;
		st -> onMem = 1;
		end = st;
	}
	else
	{
		stack* tmp = malloc(sizeof(stack));
		tmp -> node = nd;
		tmp -> next = NULL;
		tmp -> onMem = 1;
		end -> next = tmp;
		end = end -> next;
	}
}

void deleteNode(TN* node)
{
	counter--;
	TN* parent = node -> parent;
	if(parent)
		for(int i = 0; i < parent -> n + 1; i++)
		{
			if(parent -> c[i] == node)
			{
				if(parent->filenum == 209)
				//maintain relation on disk;
				parent -> childNum[i] = node -> filenum;
				//delete from Mem
				parent -> c[i] = NULL;
				break;
			}
		}
	free(node -> key);
	node -> key = NULL;
	for(int i = 0; i < 2 * DEGREE - 1; i++)
	{
		free(node -> value[i]);
		node -> value[i] = NULL;
	}
	free(node -> value);
	node -> value = NULL;
	free(node -> c);
	node -> c = NULL;
	free(node -> childNum);
	node -> childNum = NULL;
	free(node);
}

void writeToDisk(TN** node)
{
	//printf("\nwrite filenum: %d\n", (*node)->filenum);
	TN* nd = *node;
	/*
	for(int i = 0; i < nd -> n + 1; i++)
	{
		if(nd -> c[i])
		{
			printf("%d ", nd -> c[i] -> filenum);
			writeToDisk(&(nd -> c[i]));
		}

	}
	*/
	//string processing for opening data
	struct stat st = {0};
	if(stat("./storage", &st) == -1)
		mkdir("./storage", 0700);
	char buffer[50];
	char filename[50] = "./storage/";
	sprintf(buffer, "%d", nd -> filenum);
	strcat(filename, buffer);
	FILE* fp = fopen(filename, "w");
	// write the node to file
	fprintf(fp, "%d %d %d %d %d ", nd -> n, nd -> size, nd -> access, nd -> leaf, nd -> filenum);
	for(int i = 0; i < nd -> n; i++)
		fprintf(fp, "%llu %s\n", nd -> key[i], nd -> value[i]);
	if(!nd -> leaf)
		for(int i = 0; i < nd -> n + 1; i++)
			fprintf(fp, "%d\n", nd -> childNum[i]);
	fclose(fp);
	deleteNode(nd);
	*node = NULL;
}

TN* openFileFromDisk(int filenum)
{
	FILE* fp;
	if(filenum == 0)
		fp = fopen("./storage/root", "r");
	else
	{
		char buffer[50];
		char filename[50] = "./storage/";
		sprintf(buffer, "%d", filenum);
		strcat(filename, buffer);
		fp = fopen(filename, "r");
	}
	TN* nd = creatNode(DEGREE, 0, NULL);
	filenumber--;
	fscanf(fp, "%d%d%d%d%d", &(nd -> n), &(nd -> size), &(nd -> access), &(nd -> leaf), &(nd -> filenum));
	for(int i = 0; i < nd -> n; i++)
		fscanf(fp, "%llu %s", &(nd -> key[i]), nd -> value[i]);
	if(!nd -> leaf)
		for(int i = 0; i < nd -> n + 1; i++)
			fscanf(fp, "%d", &(nd -> childNum[i]));
	fclose(fp);
	//printf("filenum %d opened\n", filenum);
	return nd;
}

void writeStackToDisk()
{
	stack* tmp = NULL;
	int ti =  1 << 30;
	for(stack* t = st; t ; t = t -> next)
	{
		if(t -> onMem && ti > t -> node -> access)
		{
			tmp = t;
			ti = t -> node -> access;
		}
	}
	if(tmp)
	{
		tmp -> onMem = 0;
		writeToDisk(&(tmp -> node));
	}
	else
	{
		printf("all leave moved do disk\n");
		for(stack* t = st; t; t = t-> next)
		{
			if(t -> onMem)
			{
				t -> onMem = 0;
				writeToDisk(&(t -> node));
				return;
			}
			else
				free(t);
		}
		st = NULL;
		end = NULL;
	}
}

TN* creatNode(int t, int leaf, TN* parent)
{
	TN* nd = malloc(sizeof(TN));
	nd -> key = malloc(sizeof(unsigned long long int) * (2 * t - 1));
	nd -> value = malloc(sizeof(char*) * (2 * t - 1));
	for(int i = 0; i < 2 * t - 1; i++)
		nd -> value[i] = malloc(sizeof(char) * 129);
	nd -> c = malloc(sizeof(TN*) * 2 * t);
	nd -> childNum = calloc(2 * t, sizeof(int));
	nd -> parent = parent;
	nd -> t = t;
	nd -> leaf = leaf;
	nd -> n = 0;
	nd -> size = 0;
	nd -> access = 0;
	nd -> filenum = filenumber++;
	/*if(nd -> leaf)
		printf("leaf: %d\n", filenumber);
	*/
	counter++;
	//printf("create %d\n", nd -> filenum);
	if(nd -> leaf)
		push(nd);
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
			if(! root -> c[i])
			{
				root -> c[i] = openFileFromDisk(root -> childNum[i]);
				root -> c[i] -> parent = root;
			}
			traverse(root -> c[i]);
			printf("key: %llu, value: %s\n", root -> key[i], root -> value[i]);
		}
		if(! root -> c[root -> n])
		{
			root -> c[root -> n] = openFileFromDisk(root -> childNum[root -> n]);
			root -> c[root -> n] -> parent = root;
		}
		traverse(root -> c[root -> n]);
	}
}

void traverse_WriteToDisk(TN* root)
{
	if(root -> leaf)
	{
		writeToDisk(&root);
		return;
	}
	for(int i = 0; i < root -> n + 1; i++)
	{
		if(! root -> c[i])
		{
			root -> c[i] = openFileFromDisk(root -> childNum[i]);
			root -> c[i] -> parent = root;
		}
		traverse_WriteToDisk(root -> c[i]);
	}
	writeToDisk(&root);
}

void traversePrint(TN* root)
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
			traversePrint(root -> c[i]);
			printf("key: %llu, value: %s\n", root -> key[i], root -> value[i]);
		}
		traversePrint(root -> c[root -> n]);
	}
}

char* search(TN* root, unsigned long long int key)
{
	if(!root)
		return NULL;
	int i = 0;
	//printf("search %llu\n", key);
	//printf("filenum = %d\n", root -> filenum);
	root -> access++;
	while(root -> key[i] < key && i < root -> n)
		i++;
	if(root -> key[i] == key)
		return root -> value[i];
	if(root -> leaf)
		return NULL;
	if(!root -> c[i])
	{
		root -> c[i] = openFileFromDisk(root -> childNum[i]);
		root -> c[i] -> parent = root;
	}
	return search(root -> c[i], key);
}

void splitChild(TN* root, int i, TN* y)
{
	int tmp = y -> access;
	TN* z = creatNode(DEGREE, y -> leaf, root);
	z -> n = DEGREE - 1;
	y -> n = DEGREE - 1;

	for(int j = 0; j < DEGREE - 1; j++)
	{
		z -> key[j] = y -> key[j + DEGREE];
		strcpy(z ->value[j], y->value[j + DEGREE]);
	}
	y -> size = DEGREE- 1;
	z -> size = DEGREE- 1;
	y -> access = 0;
	z -> access = 0;
	if(! y -> leaf)
	{
		for(int j = 0; j < DEGREE; j++)
		{
			if(! y -> c[j + DEGREE])
			{
				y -> c[j + DEGREE] = openFileFromDisk(y -> childNum[j + DEGREE]);
				y -> c[j + DEGREE] -> parent = y;
			}
			z -> c[j] = y -> c[j + DEGREE];
			z -> childNum[j] = y -> c[j + DEGREE] -> filenum;
		}
		// calculate size of sub tree
		for(int j = 0; j < DEGREE; j++)
		{
			if(!y -> c[j])
			{
				y -> c[j] = openFileFromDisk(y -> childNum[j]);
				y -> c[j] -> parent = y;
			}
			y -> size += y -> c[j] -> size;
			z -> size += z -> c[j] -> size;
			y -> access += y -> c[j] -> access;
			z -> access += z -> c[j] -> access;
		}
	}
	tmp = (tmp - y -> access - z -> access) / 2;
	y -> access += tmp;
	z -> access += tmp;	
	for(int j = root -> n; j >= i+1; j--)
	{
		if(!root -> c[j])
		{
			root -> c[j] = openFileFromDisk(root -> childNum[j]);
			root -> c[j] -> parent = root;
		}
		root -> c[j + 1] = root -> c[j];
		root -> childNum[j + 1] = root -> c[j] -> filenum;
	}
	root -> c[i + 1] = z;
	root -> childNum[i + 1] = z -> filenum;
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
	root -> access++;
	if(root -> leaf)
	{
		int i = root -> n - 1;
		while(i > 0 && root -> key[i] > key)
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
		int i = root -> n - 1;
		while(i > 0 && root -> key[i] > key)
			i--;
		if(root -> key[i] == key) // find the key and update the value
		{
			strcpy(root -> value[i], value);
			return 0;
		}
		i = root -> n - 1;
		while(i >= 0 && root -> key[i] > key)
			i--;
		if(! root -> c[i + 1] && ! root -> leaf)
		{
			root -> c[i + 1] = openFileFromDisk(root -> childNum[i + 1]);
			root -> c[i + 1] -> parent = root;
		}
		if(root -> c[i + 1] -> n == 2 * DEGREE - 1)		
		{
			splitChild(root, i + 1, root -> c[i + 1]);
			if(root -> key[i + 1] == key) // find the key and update the value
			{
				strcpy(root -> value[i + 1], value);
				if(counter > TREESIZE)
					writeStackToDisk();
				return 0;
			}
			if(root -> key[i + 1] < key)
				i++;
			int tmp = insertNonFull(root -> c[i + 1], key, value);
			root -> c[i + 1] -> size += tmp;
			if(counter > TREESIZE)
				writeStackToDisk();
			return tmp;
		}
		/*
		if(! root -> c[i + 1] && ! root -> leaf)
		{
			root -> c[i + 1] = openFileFromDisk(root -> childNum[i + 1]);
			root -> c[i + 1] -> parent = root;
		}
		*/
		int tmp = insertNonFull(root -> c[i + 1], key, value);
		root -> c[i + 1] -> size += tmp;
		return tmp;
	}
}

void insert(TN** root, unsigned long long int key, char* value)
{
	if(! *root)
	{
		*root = creatNode(DEGREE, 1, NULL);
		(*root) -> key[0] = key;
		strcpy((*root) -> value[0], value);
		(*root) -> n  = 1;
		(*root) -> size++;
		(*root) -> access++;
		ROOT = *root;
	}
	else
	{
		if((*root) -> n == 2 * (*root) -> t - 1)
		{
			TN* tmp = creatNode(DEGREE, 0, NULL);
			tmp -> size = (*root) -> size; 
			tmp -> access = (*root) -> access;
			tmp -> c[0] = *root;
			tmp -> childNum[0] = (*root) -> filenum;
			// change parent of old root to new one
			(*root) -> parent = tmp;
			splitChild(tmp, 0, *root);
			tmp -> size += insertNonFull(tmp, key, value);
			*root = tmp;
			ROOT = *root;
			if(counter > TREESIZE)
				writeStackToDisk();
		}
		else
			(*root) -> size += insertNonFull(*root, key, value);
	}
}

FILE* stringProcessing(char* a)
{
	char* p1 = strtok(a, "./");
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
	free(p1);
	return fp_out;
}

void loadFileToMem(TN** root)
{
	FILE* fp = fopen("./storage/root","r");
	if(fp)
	{
		printf("load file from storage\n");
		fclose(fp);
		*root = openFileFromDisk(0);
		ROOT = *root;
		fp = fopen("./storage/filenumber", "r");
		fscanf(fp, "%d", &filenumber);
		fclose(fp);
	}
}

int main(int argc, char* argv[])
{
	printf("DEGREE: %d\n", DEGREE);
	TREESIZE = (1024 * 1024  * 1024/ NODESIZE * 3); 
	printf("%d\n", TREESIZE);
	TN* rt = NULL;
	FILE* fp = fopen(argv[1], "r");
	FILE* fp_out = stringProcessing(argv[1]);
	char command[4];
	unsigned long long int key, key2;
	char value[129];
	loadFileToMem(&rt);
	
	while(fscanf(fp, "%s", command) != EOF)
	{
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
	fclose(fp);
	fclose(fp_out);
	int rootnum = rt -> filenum;
	traverse_WriteToDisk(rt);
	struct stat st = {0};
	char buffer[50];
	char filename[50] = "./storage/";
	sprintf(buffer, "%d", rootnum);
	strcat(filename, buffer);
	rename(filename, "./storage/root");
	fp = fopen("./storage/filenumber", "w");
	fprintf(fp, "%d", filenumber);
	fclose(fp);
	//printf("%d\n", filenumber);
}
