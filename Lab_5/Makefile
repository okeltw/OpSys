obj-m := ProdCons.o

CDIR := $(shell uname -r)
MDIR := $(shell pwd)
LIB = -L/usr/include 
INC = -I/usr/include



all: ProdCons

ProdCons: ProdCons.c
	gcc ProdCons.c -lpthread -o ProdCons -std=c99 

clean:
	rm ProdCons

