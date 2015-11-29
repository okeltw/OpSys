#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

/*
	Taylor Okel
	Op Sys - Fall 2015
*/

MODULE_LICENSE("GPL");

struct workqueue_struct *queue;
struct delayed_work *dwork;

int queue_timer( void *work );

int init_module(void){
	//Landing
	printk("'Queue Timer' module is installing\n");
	printk("Queue_Timer version 1.0\n");

	//Workqueue creation
	queue = create_workqueue("queue");
	dwork = (struct delayed_work*)kmalloc(sizeof(struct delayed_work), GFP_KERNEL);
	
    //Initialize work.  Delay for 2 secs to interrupt.
	INIT_DELAYED_WORK((struct delayed_work*)dwork, queue_timer);
	queue_delayed_work(queue, dwork, 2*HZ);

	return 0;
}

int queue_timer( void *work ){
	//Do work...and stuff
	int count = 0;
	printk(KERN_EMERG "Starting...\n");
	
	//Dinging
	while(count<10){
		printk("Queue Timer Count: %i\n", count);
		count++;
	}
	
	//No interrupt, signal finish
	printk("(queue_timer) Finished main.");
	return 0;
}

//Module removed
void cleanup_module(void){
	//Interrupt, time to clean
	printk("(queue_timer) In cleanup:\n");
	if (dwork && delayed_work_pending(dwork)){
		//Work still pending.  Signal cancellation, and cancel.
		printk("(queue_timer) work pending, cancel it\n");
		cancel_delayed_work(dwork);

	}
	//Flush and destroy workqueue
	flush_workqueue(queue);
	destroy_workqueue(queue);

	//And we're done!
	printk("Queue module uninstalling\n");
}
