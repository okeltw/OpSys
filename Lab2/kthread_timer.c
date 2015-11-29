#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>

/*
	Taylor Okel
	Op Sys - Fall 2015
*/

struct task_struct *ts;
int id = 101;
//Set to notify for thread kill order
int flag=0;

int kthread_timer(void *data);

int init_module(void){
	//Landing
	printk("'Kthread Timer' module is installing\n");
	printk("Kthread_Timer version 2.0\n");

	//Start thread
	ts = kthread_run(kthread_timer, (void*)&id, "spawn");

	return 0;
}

int kthread_timer(void *data){
	//Pickup thread. Initialize count; 10 "Dings"
	int count = 0;
	printk(KERN_EMERG "Starting...\n");

	while(count<1000000000){
		count++;
		printk("Timer Count: %i\n", count);
		flag = kthread_should_stop();
		if(flag) kthread_stop(ts);
	}

	//Execution finished, cleanup.

	flag = 1;
	printk("(kthread_timer) Finished main.");
	return 0;
}

//rmmod kthread_timer called
//To check kill order, set infinite loop in kthread_timer()
void cleanup_module( void ) {	
	//Check if the thread is executing -> kill it
	printk("(kthread_timer) In cleanup:\n");
	if(!flag) kthread_stop(ts);

	//If you get here, it worked!
	printk("KThread module uninstalling\n");

	return;
}
