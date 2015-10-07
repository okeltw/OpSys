#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "queue_a.h"

#define Q &((Stream*)stream)->buffer
#define Lock &((Stream*)stream)->lock
#define Notifier &((Stream*)stream)->notifier

/* One of these per stream.
   Holds:  the mutex lock and notifier condition variables
           a buffer of tokens taken from a producer
           a structure with information regarding the processing of tokens 
           an identity
*/
typedef struct stream_struct {
   struct stream_struct *next;
   pthread_mutex_t lock;
   pthread_cond_t notifier;
   queue buffer;               /* a buffer of values of any type      */
   void *args;                 /* arguments for the producing stream */
   int id;                     /* identity of this stream            */
} Stream;

/* prod: linked list of streams that this producer consumes tokens from
   self: the producer's output stream */
typedef struct {
   Stream *self, *prod;
} Args;

typedef struct {
	int ID;
	int tokenIndex;
} ConnectEntry;

ConnectEntry consumers[10];
int numConsumers;
int currentIndex;

void IsEmpty(void);


int get(int id){
	return 0;
}

int menu(void){
	int choice = 0;

	printf("Choose an option:\n=================\n");

	printf("1) Connect consumer\n2) Disconnect Consumer\n3) Print Connected Consumers\n4) Consume\n5) Cancel Consume Request\n6) Check Buffers\n7) Quit\n>>> ");

	
	scanf("%i",&choice);

	return choice;
}

void consume(void){
	
}

void connect(int id){
	bool flag = 1;
	
	
	if (numConsumers >= 10){
		printf("\nThe maximum number of consumers (10) are already connected.\nPlease disconnect a consumer, or wait for a consumer to disconnect before proceeding.\n\n");

		return;
	}

	while(flag){			
		flag = 0;
		for(int i=0; i<numConsumers;i++){
			if(id == consumers[i].ID){
				printf("\n\nID taken. Choose a new ID.\n>>>");
				scanf("%i", &id);
				flag = 1;			
			}
		}
		
	}
	
	consumers[numConsumers].ID = id;
	consumers[numConsumers++].tokenIndex = currentIndex;
	return;
}

void disconnect(int toDelete){
	int i;
	if(numConsumers <=0){
		printf("\nNo consumers currently connected.\n\n");
		return;
	}
	for(i = 0; i <= numConsumers-1; i++){
		if(toDelete == consumers[i].ID){
			for(int k = i; k<=numConsumers-2; k++){			
				consumers[k].ID = consumers[k+1].ID;			
			}
			numConsumers--;
			printf("\nConsumer %i successfully disconnected.\n", toDelete);
			return;
		}
	}
	printf("\nConsumer ID %i not found.\n\n", toDelete);
	return;
}

void printIDs(void){
	printf("\nTaken IDs:\n");
	for(int i=0; i<numConsumers;i++){
		printf("%i\n", consumers[i].ID);
	}
	printf("\n");
}

int main(void){
	int nav = 0;
	int id = 0;
	numConsumers = 0;
	currentIndex = 0;
	Stream stream;
	stream.id = 1;

	while(nav!=7){
		nav = menu();
		if(!(nav >= 0 && nav <= 7))
		{	printf("Menu options limited to integers between 0 and 7.\n");	}


		switch (nav){
			case 0:
				break;
			case 1:
				printIDs();
				printf("=============\nAssign consumer id (integer):\n>>> ");
				scanf("%i", &id);
				connect(id);
				break;
			case 2:
				printIDs();
				printf("=============\nPlease enter the consumer ID to disconnect:\n>>>");
				scanf("%i", &id);
				disconnect(id);
				break;
			case 3:
				printIDs();
				break;
			case 4:
				printf("\nConsume\n");
				break;
			case 5:
				printf("\nCancel\n");
				break;
			case 6:
				printf("\nCheck\n");
				break;
			case 7:
				break;
		}		



	}
	
	return 0;
}
	
