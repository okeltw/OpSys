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
	int consumerID;
	int tokenIndex;
} ConnectEntry;

typedef struct {
	int nextToken;
	ConnectEntry connectedConsumer;
} connection;

int consumerIDs[];
int numConsumers;

void IsEmpty(void);


int get(int id){
	return 0;
}

int menu(void){
	int choice = 0;

	printf("Choose an option:\n=================\n");
	printf("1) Connect consumer\n2) Disconnect Consumer\n3) Consume\n4) Cancel Consume Request\n5) Check Buffers\n6) Quit\n>>> ");
	
	scanf("%i",&choice);

	return choice;
}

void connect(void){
	int id = 0;
	bool flag = 1;
	
	while(flag){	
		printf("Taken IDs:\n");
		for(int i=0; i<numConsumers;i++){
			printf("%i\n", consumerIDs[i]);
		}
		printf("=============\nAssign consumer id (integer):\n>>> ");
		scanf("%i", &id);
			
		flag = 0;
		for(int i=0; i<numConsumers;i++){
			if(id == consumerIDs[i]){
				printf("\n\nID taken. Choose a new ID.\n");
				flag = 1;			
			}
		}
		
	}
	
	consumerIDs = realloc(consumerIDs, (numConsumers + 1)*sizeof(int));
}

int main(void){
	int nav = 0;
	numConsumers = 0;
	consumerIDs = malloc(0*sizeof(int));

	while(nav!=6){
		nav = menu();
		if(!(nav >= 0 && nav <= 6))
		{	printf("Menu options limited to integers between 0 and 6.\n");	}

		switch (nav){
			case 0:
				break;
			case 1:
				connect();
				break;
			case 2:
				printf("\nDisconnect\n");
				break;
			case 3:
				printf("\nConsume\n");
				break;
			case 4:
				printf("\nCancel\n");
				break;
			case 5:
				printf("\nCheck\n");
				break;
			case 6:
				break;
		}		



	}
	
	return 0;
}
	
