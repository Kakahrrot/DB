all: db generate
db: db.c
	gcc -o db db.c
generate: generate.c
	gcc -o generate generate.c
