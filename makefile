all: db generate
db: db.c
	gcc -g -o db db.c
generate: generate.c
	gcc -o generate generate.c
clear:
	rm storage/*
