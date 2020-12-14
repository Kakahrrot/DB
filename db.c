#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEGREE 1024 * 1024
// remember to close file and output all nodes to disk and rename root

typedef struct treenode
{
	unsigned long long int *key;
	char** value;
	int t;
	struct treenode** c;

	//remember to mainain parent!!
	struct treenode* parent;
	int filenum;
	int* childNum;// -1 means not onMem!!
	int n; //current number of keys
	int size;//current number of key-value pair in the sub-tree
	int access;
	int leaf;
}TN;


typedef struct st{
	TN* node;
	int onMem;
	struct st* next;
}stack;

TN* creatNode(int t, int leaf, TN* root ,TN* parent);



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

int NODESIZE = sizeof(TN) + (sizeof(unsigned long long int) + sizeof(char*) + sizeof(char) * 129) * (2 * DEGREE - 1) + (sizeof(TN*) + sizeof(int)) * 2 * DEGREE;
int TREESIZE = 0;//((1024 * 1024 * 1024 / NODESIZE * 3) % 1000 ) * 1000; 

int counter = 0;

void deleteNode(TN* node)
{
	TN* parent = node -> parent;
	for(int i = 0; i < parent -> n + 1; i++)
	{
		if(parent -> c[i] == node)
		{
			//maintain relation on disk;
			parent -> childNum[i] = node -> filenum;
			parent -> c[i] = NULL;//delete from Mem
			break;
		}
	}
	free(node -> key);
	for(int i = 0; i < 2 * DEGREE - 1; i++)
		free(node -> value[i]);
	free(node -> value);
	free(node -> c);
	free(node -> childNum);
	free(node);
}

void writeToDisk(TN* nd)
{
	struct stat st = {0};
	if(stat("./storage", &st) == -1)
		mkdir("./storage", 0700);
	char buffer[50];
	char filename[50] = "./storage/";
	sprintf(buffer, "%d", nd -> filenum);
	strcat(filename, buffer);
	FILE* fp = fopen(filename, "w");
	// write the TN node to file
	fprintf(fp, "%d %d %d %d %d ", nd -> n, nd -> size, nd -> access, nd -> leaf, nd -> filenum);
	for(int i = 0; i < nd -> n; i++)
		fprintf(fp, "%llu %s\n", nd -> key[i], nd -> value[i]);
	if(!nd -> leaf)
		for(int i = 0; i < nd -> n + 1; i++)
			fprintf(fp, "./storage/%d\n", nd -> childNum[i]);
	fclose(fp);
	deleteNode(nd);
}

TN* openFileFromDisk(int filenum)
{
	char buffer[50];
	char filename[50] = "./storage/";
	sprintf(buffer, "%d", filenum);
	strcat(filename, buffer);
	FILE* fp = fopen(filename, "r");
	TN* nd = creatNode(DEGREE, 0, NULL, NULL);
	fscanf(fp, "%d%d%d%d%d", &(nd -> n), &(nd -> size), &(nd -> access), &(nd -> leaf), &(nd -> filenum));
	for(int i = 0; i < nd -> n; i++)
		fscanf(fp, "%llu %s", &(nd -> key[i]), nd -> value[i]);
	if(!nd -> leaf)
		for(int i = 0; i < nd -> n + 1; i++)
			fscanf(fp, "./storage/%d\n", &(nd -> childNum[i]));
	fclose(fp);
	return nd;

}
//check num of node on stack == num of node deleted???
int writeStackToDisk()
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
		writeToDisk(tmp -> node);
		return 1; // OK!
	}
	else
	{
		for(stack* t = st; t; t = t-> next)
		{
			if(t -> onMem)
			{
				t -> onMem = 0;
				writeToDisk(t -> node);
				return 1;
			}
			else
				free(t);
		}
		st = NULL;
		end = NULL;
		return 0;// need to free non-leaf node QQ
	}
}

void addToStack(TN* root)
{
	//add non-leaf node to stack and wait for being deleted
	if(!root)
		return;
	if(!root -> c[0])
	{
		push(root);
		return;
	}
	else
	{
		for(int i = 0; i < root -> n + 1; i++)
			addToStack(root -> c[i]);
	}
	
}

TN* creatNode(int t, int leaf, TN* root, TN* parent)
{
	if(counter >= TREESIZE)
		if(!writeStackToDisk())
		{
			addToStack(root);
			writeStackToDisk();
		}
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
	nd -> filenum = counter++;
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

void traverse_loop(TN* root)
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



// test if the child is NULL? if it is NULL go find the disk!!
char* search(TN* root, unsigned long long int key)
{
	int i = 0;
	root -> access++;
	while(root -> key[i] < key && i < root -> n)
		i++;
	if(root -> key[i] == key)
		return root -> value[i];
	if(root -> leaf)
		return NULL;
	if(!root -> c[i]  && ! root -> leaf)
	{
		root -> c[i] = openFileFromDisk(root -> childNum[i]);
		root -> c[i] -> parent = root;
	}
	return search(root -> c[i], key);
}

//remember to maintain parent ralationship
void splitChild(TN* root, int i, TN* y)
{
	int tmp = y -> access;
	TN* z = creatNode(DEGREE, y -> leaf, y, root);
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

// test if the child is NULL? if it is NULL go find the disk!!
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
				return 0;
			}
			if(root -> key[i + 1] < key)
				i++;
		}
		int tmp = insertNonFull(root -> c[i + 1], key, value);
		root -> c[i + 1] -> size += tmp;
		root -> c[i + 1] -> access++;
		return tmp;
	}
}

// test if the child is NULL? if it is NULL go find the disk!!
void insert(TN** root, unsigned long long int key, char* value)
{
	if(! *root)
	{
		*root = creatNode(DEGREE, 1, NULL, NULL);
		(*root) -> key[0] = key;
		strcpy((*root) -> value[0], value);
		(*root) -> n  = 1;
		(*root) -> size++;
		(*root) -> access++;
	}
	else
	{
		if((*root) -> n == 2 * (*root) -> t - 1)
		{
			TN* tmp = creatNode(DEGREE, 0, *root, NULL);
			tmp -> size = (*root) -> size; 
			tmp -> access = (*root) -> access;
			tmp -> c[0] = *root;
			// change parent of old root to new onw
			(*root) -> parent = tmp;
			splitChild(tmp, 0, *root);
			//int i = 0;
			//if(tmp ->key[0] < key)
			//	i++;
			//tmp -> size += insertNonFull(tmp -> c[i], key, value);
			tmp -> size += insertNonFull(tmp, key, value);
			tmp -> access++;
			*root = tmp;
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

int main(int argc, char* argv[])
{
	printf("%d\n", NODESIZE);
	TREESIZE = (1024 * 1024 * 1024 / NODESIZE * 3); 
	printf("%d\n", TREESIZE);
	//return 1;
	TN* rt = NULL;
	//FILE* fp = fopen("test.input", "r");
	//FILE* fp_out = fopen("test.output", "w");
	FILE* fp = fopen(argv[1], "r");
	FILE* fp_out = stringProcessing(argv[1]);
	char command[4];
	unsigned long long int key, key2;
	char value[129];
	while(fscanf(fp, "%s", command) != EOF)
	{
		//printf("%s %d\n", command, counter++);
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
	traverse(rt);
	printf("size = %d\n", rt -> size);
}
