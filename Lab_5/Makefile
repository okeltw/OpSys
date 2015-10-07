obj-m := ProdCons.o 07-prod-cons.o

CDIR := $(shell uname -r)
MDIR := $(shell pwd)
LIB = -L/usr/include 
INC = -I/usr/include



all: ProdCons

ProdCons: ProdCons.c
	gcc ProdCons.c queue_a.c -lpthread -o ProdCons -std=c99 

07-prod-cons: 07-prod-cons.c
	gcc 07-prod-cons.c queue_a.c -lpthread -o	07-prod-cons -std=c99
	

clean:
	rm ProdCons

fresh:
	rm ProdCons
	gcc ProdCons.c -lpthread -o ProdCons -std=c99


