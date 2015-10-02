#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>

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
