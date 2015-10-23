#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h> 
#include <linux/slab.h>
#include <linux/acpi.h>
#include <asm/uaccess.h>
#include <asm/i387.h>
#include <acpi/actypes.h>

MODULE_LICENSE("GPL");

/*
	Taylor Okel
	Op Sys - Fall 2015
	Lab 4
	Version 1.0
*/

struct workqueue_struct *queue;
struct delayed_work *dwork;
acpi_status status;
acpi_handle handle;
union acpi_object *bif_result, *bst_result;
int power_unit,last_full_cap,bat_low,bat_state, bat_remain_cap, bat_present_vol, bat_present_rate;
char unit[10];
int x;
int delay = 30;
int count = 0;
struct acpi_buffer bst_buffer = { ACPI_ALLOCATE_BUFFER, NULL };
struct acpi_buffer bif_buffer = { ACPI_ALLOCATE_BUFFER, NULL };

/*
<summary>
	[Function to attach to the work queue]
	Uses ACPI specification to monitor battery status and display non-intrusive updates.
</summary>
<parameters>
	work - the work to be done
</parameters>
<output>
	V1 will output kernel messages.
	Further versions may write to files or utilize a java applet.
</output>
*/
int battcheck(struct work_struct *work){
	//Todo
	return 0;
}

/*
	Initialize.  Set up work queue, attach battcheck, etc.
*/
int init_module(void){
	printk("[Battcheck] Module is loading...\n");
	queue = create_workqueue("queue"); //set up queue
	dwork = (struct delayed_work*)kmalloc(sizeof(struct delayed_work), GFP_KERNEL); //allocate work queue
	INIT_DELAYED_WORK((struct delayed_work*) dwork, battcheck)); // attach battcheck to work
	queue_delayed_work(queue, dwork, HZ); // schedule work
	return 0;
}

/*
	Cleanup.  Flush work queue; cancel remaining work.
*/
void cleanup_module(void){
	flush_workqueue(queue); //Flush pending work
	if(dwork && delayed_work_pending(dwork)){
		cancel_delayed_work(dwork);  //Cancel running work, if any work is running
		flush_workqueue(queue);		 //Flush again
	}
	destroy_workqueue(queue); //Destroy queue to free resources
	printk(KERN_EMERG "[Battcheck] Module unloaded successfully");
}
