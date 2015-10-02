#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>


MODULE_LICENSE("GPL");

/*
	Taylor Okel
	Jeff Tobe
	Op Sys - Fall 2015
	Lab 3
*/

struct semaphore avail;

//Thread Management
int flag[2] = { 0, 0};
struct task_struct *t1;
struct task_struct *t2;


//Execution
int idx = 0;
int arr[1000000] = {0};
int stat[1000000] = {0};
int cs1 = 0, cs2 = 0;

int incrementer(void *data){
	int id = (int)data;
	int v;	

	printk(KERN_INFO "Consumer TID %d\n", id);



	while(idx < 1000000){
		if(!down_interruptible(&avail)){
			arr[idx++] += 1;
			up(&avail);
		}		

		if((int)id == 1)
			cs1++;
		else if((int)id == 2)
			cs2++;

		schedule();
	}

	if((int)id == 1)
		flag[0] = 1;
	else if((int)id == 2)
		flag[1] = 1;
	
	printk(KERN_INFO "Thread %d Finished\n", id);

	
	
	return 0;
}

int init_module(void){
	int id1=1, id2=2;
	t1 = kthread_create(incrementer, (void*)id1, "inc1");
	t2 = kthread_create(incrementer, (void*)id2, "inc2");

	sema_init(&avail, 1);

	if(t1 && t2){
		printk(KERN_INFO "\n\nStarting...\n");
		wake_up_process(t1);
		printk(KERN_INFO "Process t1 awake\n");
		wake_up_process(t2);
		printk(KERN_INFO "Process t2 awake\n");
	} else {
		printk(KERN_EMERG "Error\n");
	}
	return 0;
}



void cleanup_module(void){
	
	int x;	
	for(x=0; x<1000000; x++){
		if(arr[x] > 1){		
			stat[x]++;
		}
		if(stat[x] > 0){
			printk(KERN_INFO "stat[%i] > 1; %i \ncs1 = %d, cs2 = %d\ncs1 + cs2 = %d", x, arr[x], 						cs1, cs2, cs1+cs2);
			
		}
	}

	if(!flag[0]) kthread_stop(t1);
	if(!flag[1]) kthread_stop(t2);

	printk(KERN_INFO "Mult Threads module uninstalling\ncs1 = %d\ncs2 = %d\nCount = %d\nIDX = %d\n",cs1,cs2,cs1+cs2, idx);

	return;
		
}

