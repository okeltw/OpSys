#ifndef _QUEUE_A
#define _QUEUE_A

typedef struct {
   long *data;
   int tail, head, size, nelem;
} queue;

void init_queue(queue*);
void enqueue(queue*, long);
long dequeue(queue*);
bool isEmpty(queue*);
int nelem(queue*);
void print (queue*,const char*,int);
void destroy_queue(queue*);
long peek(queue*,int);

#endif
