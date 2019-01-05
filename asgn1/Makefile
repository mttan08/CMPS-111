# Makefile for asgn1

argshell: argshell.o
	cc -o argshell argshell.o lex.yy.c -lfl

argshell.o : argshell.c
	cc -c argshell.c

lex.yy.c: shell.l
	flex shell.l

clean :
	rm -f argshell argshell.o lex.yy.o
