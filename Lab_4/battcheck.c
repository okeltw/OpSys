#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <asm/uaccess.h>
#include <asm/i387.h>
#include <acpi/actypes.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>

MODULE_LICENSE("GPL");

/*
	Taylor Okel
	Op Sys - Fall 2015
	Lab 4
	Version 1.0
*/

#define BUFFER_SIZE 512


struct workqueue_struct *queue;
struct delayed_work *dwork;


//BIF
struct proc_dir_entry *proc_entry_info, *root_dir_info = NULL;
int power_unit,design_capacity,last_full_cap,bat_tech,design_voltage,bat_warn,bat_low, gran1, gran2;
char *model, *serial, *type, *oem;

//BST
struct proc_dir_entry *proc_entry_stat, *root_dir_stat = NULL;
int bat_state, bat_present_rate, bat_remain_cap, bat_present_vol;

char unit[10];
int x;
int delay = 30;
int count = 0;

int state = 0; //To signal when to display messages
int batLow_msg = -1; //Signals battery low message

//Java applet
int temp;
char stat_buffer[BUFFER_SIZE];
char info_buffer[BUFFER_SIZE];
bool infoFlag = false, statFlag = false;
int previous_vol = 0;
int custom_rate = 0;

ssize_t infoRead(struct file *fd, char __user *buf, size_t c, loff_t *off) {
	if(infoFlag){
		infoFlag = false;
		return 0;
	}

	int len = 0;

	//printk("%d %d %d %d %d %d %d %d %d %s %s %s %s\n", power_unit, design_capacity, last_full_cap, bat_tech, design_voltage, bat_warn, bat_low, gran1, gran2, model, serial, type, oem);
	sprintf(info_buffer, "%d %d %d %d %d %d %d %d %d %s %s %s %s\n", power_unit, design_capacity, last_full_cap, bat_tech, design_voltage, bat_warn, bat_low, gran1, gran2, model, serial, type, oem);
	len = strlen(info_buffer);
	if(copy_to_user(buf, info_buffer, len)) return -EFAULT;

	infoFlag = true;
	return len;
}

ssize_t statRead(struct file *fd, char __user *buf, size_t c, loff_t *off) {
	if(statFlag){
		statFlag = false;
		return 0;
	}

	int len = 0;
	sprintf(stat_buffer, "%d %d %d %d\n", bat_state, bat_present_rate, bat_remain_cap, bat_present_vol);
	len = strlen(stat_buffer);
	if(copy_to_user(buf, stat_buffer, len)) return -EFAULT;

	statFlag = true;
	return len;
}

static const struct file_operations infoOp = {
	.owner = THIS_MODULE,
	.read = infoRead
};

static const struct file_operations statOp = {
	.owner = THIS_MODULE,
	.read = statRead
};

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
	Further versions may utilize a java applet.
</output>
*/
int battcheck(struct work_struct *work){
  acpi_status status;
  acpi_handle handle;
  union acpi_object *bif_result, *bst_result;
  struct acpi_buffer bst_buffer = { ACPI_ALLOCATE_BUFFER, NULL };
  struct acpi_buffer bif_buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	// status is an integer that holds the return value from the functions.
	status = acpi_get_handle(NULL, "\\_SB_.PCI0.BAT0", &handle); //grab the handle for the battery acpi.

	status = acpi_evaluate_object(handle, "_BST", NULL, &bst_buffer); //use the handle to fill the bst_buffer

	if(ACPI_FAILURE(status)){
		printk(KERN_EMERG "[Battcheck] acpi call could not get the handle.\n"); // If the status indicates ACPI failure, indicate so and failout.
		return -1;
	}



	// BST == info about battery state
	bst_result = bst_buffer.pointer; // populate the result with the pointer
	if(bst_result){
		bat_state = bst_result->package.elements[0].integer.value;
		bat_present_rate = bst_result->package.elements[1].integer.value;
		bat_remain_cap = bst_result->package.elements[2].integer.value;
		bat_present_vol = bst_result->package.elements[3].integer.value;


		if(bat_remain_cap <= bat_low && batLow_msg!=1){
			printk(KERN_INFO "[Battcheck] Battery is low\n");
			batLow_msg = 1;
		}
		else if(bat_remain_cap == last_full_cap && batLow_msg!=0) {
			printk(KERN_INFO "[Battcheck] Battery is full\n");
			batLow_msg = 0;
		}


		kernel_fpu_begin(); //Start the fpu to do proper calculations

		x = bat_remain_cap*100;
		x = x/last_full_cap;

    if(bat_present_rate < 0){
      custom_rate = (previous_vol - bat_present_vol) / 60;
      bat_present_rate = custom_rate > 0 ? custom_rate : custom_rate * -1;
      previous_vol = bat_present_vol;
    }

    kernel_fpu_end();

		if(bat_state == 1 && state !=1){ //Discharging
			 //If we are not in discharge state, send the message.
			printk(KERN_INFO "[Battcheck] Battery is discharging...%d%% battery remaining\n", x);
			state = 1;
		}
		else if(bat_state==2 && state!=2){ //Charging
			printk(KERN_INFO "[Battcheck] Battery is charging...%d%% battery available\n", x);
			state = 2;
		}
		else if(bat_state != 1 && bat_state != 2 ){
			printk(KERN_EMERG "[Battcheck] Battery is critically low; Capacity = %d%% \n", x);
			kernel_fpu_end();
			queue_delayed_work(queue, dwork, delay*HZ); //queue the message to appear at the specified delay

			return 0;
		}

		kfree(bst_result);
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

	// BIF == info about battery
	status = acpi_get_handle(NULL, "\\_SB_.PCI0.BAT0", &handle); //grab the handle for the battery acpi.
	status = acpi_evaluate_object(handle, "_BIF", NULL, &bif_buffer); // use the handle to fill the bif_buffer
	bif_result = bif_buffer.pointer; // populate the result with the pointer
	if(bif_result){
		power_unit = bif_result->package.elements[0].integer.value; // fill power unit specification

		if(power_unit==1) strcpy(unit, "mWh");  // Identify power units
		else strcpy(unit, "mAh");

		design_capacity = bif_result->package.elements[1].integer.value;
		last_full_cap = bif_result->package.elements[2].integer.value;
		bat_tech = bif_result->package.elements[3].integer.value;
		design_voltage = bif_result->package.elements[4].integer.value;
		bat_warn = bif_result->package.elements[5].integer.value;
		bat_low = bif_result->package.elements[6].integer.value;
		gran1 = bif_result->package.elements[7].integer.value;
		gran2 = bif_result->package.elements[8].integer.value;

		model = bif_result->package.elements[9].string.pointer;
		serial = bif_result->package.elements[10].string.pointer;
		type = bif_result->package.elements[11].string.pointer;
		oem = bif_result->package.elements[12].string.pointer;

		kfree(bif_result);
	}

	proc_entry_info = proc_create("battery_info.txt", 438, NULL, &infoOp);
	strcpy(info_buffer, "Initialize\n");
	if(proc_entry_info == NULL){
		printk(KERN_ERR "[Battcheck] Couldn't create proc entry for info\n");
		return -ENOMEM;
	}
	else
		printk(KERN_INFO "[Battcheck] Proc entry for info created.\n");

	proc_entry_stat = proc_create("battery_stat.txt", 438, NULL, &statOp);
	strcpy(stat_buffer, "Initialize\n");
	if(proc_entry_stat == NULL){
		printk(KERN_ERR "[Battcheck] Couldn't create proc entry for stat\n");
		return -ENOMEM;
	}
	else
		printk(KERN_INFO "[Battcheck] Proc entry for stat created.\n");

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
	printk(KERN_EMERG "[Battcheck] Module unloaded successfully\n");
}
