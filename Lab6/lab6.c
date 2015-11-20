
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

/*
  Taylor Okel
  Operating Systems
  Fall 2015
  Lab 6

  Credit:
  Prof Franco - Lab 6 description
  Kevin Farley - general ideas and direction, run and stop scripts
*/

MODULE_LICENSE("GPL");

#define FNAME "interface"

//From lab6 assistance:
struct cdev *kernel_cdev;
dev_t dev_no;
static int Major;
wait_queue_head_t queue;

// Suggested from K. Farley
static int written, read; //written - sizeof area containing data
                          //read - printing

// struct rw_semaphore {
//   long count;
//   raw_spinlock_t wait_lock;
//   struct list_head wait_list;
// }

struct device {
  int array[100];
  struct rw_semaphore rw_sema;
} Device;



//semaphore
/********************************************************/
// void init_rwsem(struct rw_semaphore); //Init semaphore
//
// void down_read(struct rw_semaphore *sem); //Hold semahore for reading, sleep if not available
//
// void up_read(struct rw_semaphore *sem); // Release semaphore for reading
//
// void down_write(struct rw_semaphore *sem); //Hold semaphore for writing, sleep if not available
//
// void down_write_trylock(struct rw_semaphore *sem); // Hold semaphore for writing, error if not available
//
// void up_write(struct rw_semaphore *sem;) // Release semaphore for writing
/********************************************************/

//Device interface
/********************************************************/
ssize_t Read(struct file *filp, char *buff, size_t count, loff_t *offp){
  unsigned long ret;

  printk("[lab6|read] Buffer (start) = %s\n", buff);
  down_read(&Device.rw_sema);
  printk("[lab6|read] Grabbed semaphore\n");
  wait_event_timeout(queue, 0,20*HZ); // Chill in critical section
  printk("[lab6|read] Ding! Timer's up.\n");

  ret = copy_to_user(buff, Device.array, count);

  printk("[lab6|read] Read successfully\n");
  printk("[lab6|read] Read = %d, Written = %d, Count = %d\n", read, written, count);
  up_read(&Device.rw_sema);
  printk("[lab6|read] Released semaphore\n");
  return count;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp){
  unsigned long ret;

  printk(KERN_INFO "[lab6|write] Begin Write\n");

  down_write(&Device.rw_sema);
  printk("[lab6|write] Grabbed semaphore\n");

  wait_event_timeout(queue, 0, 15*HZ); // Chill in critical section
  printk("[lab6|write] Ding! Timer's up.\n");
  count = (count > 99) ? 99:count;
  ret = copy_from_user(Device.array, buff, count);
  written += count;

  printk("[lab6|write] Wrote successfully: %s\n", buff);
  printk("[lab6|write] Read = %d, Written = %d, Count = %d\n", read, written, (int)count);

  up_write(&Device.rw_sema);
  printk("[lab6|write] Released semaphore\n");

  return count;
}

int open(struct inode *inode, struct file *filp){
  printk(KERN_INFO "[lab6|open] Read = %d, Written = %d\n", read, written);
  read = written;
  return 0;
}

int release(struct inode *inode, struct file *filp) {
  //Deallocation happens in rmmod
  printk(KERN_INFO "[lab6|release] Read=%d, Written=%d\n", read, written);
  return 0;
}

// Now attach the operations:
struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = Read,
  .write = write,
  .open = open,
  .release = release
};
/********************************************************/

int init_module(void) {
  printk(KERN_EMERG "[lab6] Loading module...\n");

  int ret;

  kernel_cdev = cdev_alloc();
  kernel_cdev->ops = &fops;
  kernel_cdev->owner = THIS_MODULE;
  ret = alloc_chrdev_region(&dev_no, 0,1, FNAME);
  if (ret < 0) {
    printk(KERN_EMERG "[lab6] Major number allocation has failed\n");
    return ret;
  }


  Major = MAJOR(dev_no);
  printk(KERN_EMERG "[lab6] Major number: %d, %d\nRet: %d", dev_no, Major, ret);

  if(MKDEV(Major, 0) != dev_no)
    printk(KERN_INFO "An error occured.\n");

  ret = cdev_add(kernel_cdev, dev_no, 1);
  if (ret < 0) {
    printk(KERN_INFO "unable to allocate cdev\n");
    return ret;
  }

  init_rwsem(&Device.rw_sema);
  init_waitqueue_head(&queue);

  return 0;
}

void cleanup_module(void){
  printk(KERN_EMERG"[lab6] in cleanup\n");
  cdev_del(kernel_cdev);
  unregister_chrdev_region(dev_no, 1);
  unregister_chrdev(Major, FNAME);
  printk(KERN_EMERG "[lab6] Module unloaded.\n");
}
