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
	// status is an integer that holds the return value from the functions.
	status = acpi_get_handle(NULL, "\\_SB_.PCI0.BAT0", &handle); //grab the handle for the battery acpi.
	status = acpi_evaluate_object(handle, "_BIF", NULL, &bif_buffer); // use the handle to fill the bif_buffer
	status = acpi_evaluate_object(handle, "_BST", NULL, &bst_buffer); //use the handle to fill the bst_buffer
	
	if(ACPI_FAILURE(status)){
		printk(KERN_EMERG "[Battcheck] acpi call could not get the handle.\n"); // If the status indicates ACPI failure, indicate so and failout.
		return -1;
	}
	
	// BIF == info about battery
	bif_result = bif_buffer.pointer; // populate the result with the pointer
	if(bif_result){
		power_unit = bif_result->package.elements[0].integer.value; // fill power unit specification
		
		if(power_unit==1) strcpy(unit, "mWh");  // Identify power units
		else strcpy(unit, "mAh");
		
		last_full_cap = bif_result->package.elements[2].integer.value; // check last full capacitance
		bat_low = bif_result->package.elements[6].integer.value; // Check the battery low level
	}
	
	// BST == info about battery state
	bst_result = bst_buffer.pointer; // populate the result with the pointer
	if(bst_result){
		bat_remain_cap = bst_result->package.elements[2].integer.value;
		
		if(bat_remain_cap == bat_low)
			printk(KERN_INFO "[Battcheck] Battery is low\n");
		else if(bat_remain_cap == last_full_cap)
			printk(KERN_INFO "[Battcheck] Battery is full\n");
			
		bat_state = bst_result->package.elements[0].integer.value;
		if(bat_state == 1){ //Discharging
			kernel_fpu_begin(); //Start the fpu to do proper calculations
			x = bat_remain_cap*100;
			x = x/last_full_cap;
			printk(KERN_INFO "[Battcheck] Battery is discharging...%d%%dmes battery remaining\n", x);
			kernel_fpu_end(); //Done with fpu
		}
		else if(bat_state==2){ //Charging
			kernel_fpu_begin(); //Start the fpu to do proper calculations
			x = bat_remain_cap*100;
			x = x /last_full_cap;
			printk(KERN_INFO "[Battcheck] Battery is charging...%d%% battery available\n", x);
			kernel_fpu_end(); //Done with fpu
		}
		else{
			kernel_fpu_begin(); //Start the fpu to do proper calculations
			x = bat_remain_cap*100;
			x = x /last_full_cap;
			printk(KERN_EMERG "[Battcheck] Battery is critically low; Capacity = %d%% \n", x);
			kernel_fpu_end(); //Done with fpu
			
			queue_delayed_work(queue, dwork, delay*HZ); //queue the message to appear at the specified delay
			return 0;
		}
	}
	
	//DEBUG
	//printk(KERN_EMERG "[Battcheck] Battery State: %d\nBattery Remaining: %d %s\n", bat_state, bat_remain_cap, unit);
	//printk(KERN_EMERG "[Battcheck] Power Unit: %d\nLast Full Charge: %d %s\nLow Battery:%d %s", power_unit, last_full_cap, unit, bat_low, unit);
	queue_delayed_work(queue, dwork, HZ); //Schedule the next check
	return 0;
	
}

/*
	Initialize.  Set up work queue, attach battcheck, etc.
*/
int init_module(void){
	printk(KERN_EMERG "[Battcheck] Module is loading...\n");
	queue = create_workqueue("queue"); //set up queue
	dwork = (struct delayed_work*)kmalloc(sizeof(struct delayed_work), GFP_KERNEL); //allocate work queue
	INIT_DELAYED_WORK((struct delayed_work*) dwork, battcheck); // attach battcheck to work
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
