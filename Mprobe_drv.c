

/* ----------------------------------------------- DRIVER Mprobe --------------------------------------------------
 
 Basic driver example to show skelton methods for several file operations.
TSC code reference:  https://www.mcs.anl.gov/~kazutomo/rdtsc.html
 ----------------------------------------------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include<linux/init.h>
#include<linux/moduleparam.h>

#define DEVICE_NAME                 "Mprobe"  // device name to be created and registered
#define SIZE_OF_BUF                   10
 int errno;
int is_registered;
static struct kprobe kp;
typedef struct {
	uint64_t TSC;
	unsigned long  ADDR;
	pid_t PID;
	int VALUE;
}data, *Pdata;

 struct ring_buf{
	 Pdata ptr;
	int   head;
	int   tail;
	unsigned long count;
	unsigned long loss;
};
struct ring_buf *Pring_buf;
/* per device structure */
struct Mprobe_dev {
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	int location;
	int current_write_pointer;
} *Mprobe_devp;

static dev_t Mprobe_dev_number;      /* Allotted device number */
struct class *Mprobe_dev_class;          /* Tie with the device model */
static struct device *Mprobe_dev_device;

static char *user_name = "CSE530_Assignment1";
module_param(user_name,charp,0000);	//to get parameter from load.sh script to greet the user

/*Time Stamp Counter code*/
#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}
#elif defined(__x86_64__)


static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#endif

/*
* Open Mprobe driver
*/
int Mprobe_driver_open(struct inode *inode, struct file *file)
{
	struct Mprobe_dev *Mprobe_devp;

	/* Get the per-device structure that contains this cdev */
	Mprobe_devp = container_of(inode->i_cdev, struct Mprobe_dev, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = Mprobe_devp;
	Pring_buf = kmalloc(sizeof(struct ring_buf), GFP_KERNEL);
		
	if (!Pring_buf) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}
	
	if(!(Pring_buf->ptr = kmalloc(sizeof(data)* SIZE_OF_BUF, GFP_KERNEL)))
	{		
		printk("Bad Kmalloc\n");
		 return -ENOMEM;
	}
		
		Pring_buf->head =0;
		Pring_buf->tail =0;
		Pring_buf->count=0;
		Pring_buf->loss =0;
		
		is_registered = 0;
	
	printk("\n%s is openning \n", Mprobe_devp->name);
	return 0;
}

/*
 * Release Mprobe driver
 */
int Mprobe_driver_release(struct inode *inode, struct file *file)
{
	struct Mprobe_dev *Mprobe_devp = file->private_data;
	
	kfree(Pring_buf->ptr);
	kfree(Pring_buf);
	unregister_kprobe(&kp);
	is_registered = 0;
	printk("\n%s is closing\n", Mprobe_devp->name);
	
	return 0;
}

/*
 * Write to Mprobe driver
 */
ssize_t Mprobe_driver_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
	int *buffer_addr;
	int ret,retr;
	void*pt;
	
	struct Mprobe_dev *Mprobe_devp = file->private_data;
	
	if(!(buffer_addr = kmalloc(sizeof(int), GFP_KERNEL)))
	{		printk("Bad Kmalloc\n");
		 return -ENOMEM;
	}	
	memset(buffer_addr, 0, sizeof (int));
	if(copy_from_user(buffer_addr, buf, count)) 
		return -EFAULT;
	Mprobe_devp->location = *(buffer_addr);
	pt = (void*)Mprobe_devp->location;
	printk("address from the user to insert the probe = 0x%p",(void*)Mprobe_devp->location);
	
	kp.addr = (kprobe_opcode_t *)pt 			/*kallsyms_lookup_name("ht530_driver_write")+*/;
//printk("value of is_registered = %d", is_registered);
	if(is_registered ==0)
	{
		retr= register_kprobe(&kp);
		if (retr < 0) {
		printk(KERN_INFO "register_kprobe 1failed, returned %d\n", retr);
		return retr;}
		else
		{printk ("driver is registed");
		is_registered =1;
		}
	}
	else
	{	unregister_kprobe(&kp);
		is_registered = 0;
//printk("unregistered\n");
		ret = register_kprobe(&kp);
		if (ret < 0) {
		printk(KERN_INFO "register_kprobe 2 failed, returned %d\n", ret);
		return ret;}
		else
		{is_registered = 1;
	//	printk("registed back\n");
		}
	}	

	return 0;
}

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	int value;
	uint64_t tsc;
	pid_t pid;
	Pdata pptr;
	 if(!(pptr = kmalloc(sizeof(data), GFP_KERNEL)))
	{               printk("Bad Kmalloc\n");
                 return -ENOMEM;
	}	

	printk(KERN_INFO "<%s> pre_handler: p->addr = 0x%p, ip = %lx,"
			" flags = 0x%lx\n",
		p->symbol_name, p->addr, regs->ip, regs->flags);

//printk(" pt_regs.ebp = 0x%p\n", regs->ebx);

	tsc =rdtsc();
   // printk("TSC = %llu \n", tsc);
	 value = *((int*)kp.addr);
	// printk("VALUE = 0x%d \n", value);
	  pid = task_pid_nr(current);
          //      printk("PID = %d\n", pid);
	  pptr->TSC = tsc;
          pptr->VALUE= value;
          pptr->PID= pid;
          pptr->ADDR=(unsigned long)p->addr;


/* writing into the ring buffer*/				
	 if(Pring_buf->ptr )
	 {

		 memcpy(&(Pring_buf->ptr[Pring_buf->tail]), pptr, sizeof(data));
		 printk("copied in to the ring_buf\n");
		 Pring_buf->tail = (Pring_buf->tail +1) % SIZE_OF_BUF;
		  if(Pring_buf->tail == Pring_buf->head)
		  {
			  Pring_buf->loss++;
		  }
		  else
		  {
			  Pring_buf->count++;
		  }
 	 }
	 
	 
	return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{

	printk(KERN_INFO "<%s> post_handler: p->addr = 0x%p, flags = 0x%lx\n",
		p->symbol_name, p->addr, regs->flags);
}


static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_INFO "fault_handler: p->addr = 0x%p, trap #%dn",
		p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}
/*
 * Read to Mprobe driver
 */
ssize_t Mprobe_driver_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
	
	int bytes_read;	
	if(Pring_buf->count >= 1)
	{
	int var= Pring_buf->head;
	//printk(" in read PID = %d ", Pring_buf -> ptr[var].PID); 
	//if (copy_to_user((Pdata)buf, &(Pring_buf->ptr[Pring_buf->head]) , sizeof(data))) 
	if (copy_to_user((Pdata)buf, &(Pring_buf->ptr[var]) , sizeof(data))) 
        	{	printk("unable to copy to  the user");
			return -EFAULT;
		}
	bytes_read = sizeof(data);
	Pring_buf->head = (Pring_buf->head + 1)% SIZE_OF_BUF;
	return bytes_read;
	}
	else
	{
		printk(" invalid read operation from the ring buffer\n");
		errno = EINVAL;
		return -1;
	  
	}

}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations Mprobe_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= Mprobe_driver_open,        /* Open method */
    .release	= Mprobe_driver_release,     /* Release method */
    .write		= Mprobe_driver_write,       /* Write method */
    .read		= Mprobe_driver_read,        /* Read method */
};

/*
 * Driver Initialization
 */
int __init Mprobe_driver_init(void)
{
	int ret;
	int time_since_boot;
    
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&Mprobe_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	Mprobe_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	
		/* Allocate memory for the per-device structure */
	Mprobe_devp = kmalloc(sizeof(struct Mprobe_dev), GFP_KERNEL);
		
	if (!Mprobe_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	
	/* Request I/O region */
	sprintf(Mprobe_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&Mprobe_devp->cdev, &Mprobe_fops);
	Mprobe_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&Mprobe_devp->cdev, (Mprobe_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	Mprobe_dev_device = device_create(Mprobe_dev_class, NULL, MKDEV(MAJOR(Mprobe_dev_number), 0), NULL, DEVICE_NAME);		

	time_since_boot=(jiffies-INITIAL_JIFFIES)/HZ;//since on some systems jiffies is a very huge uninitialized value at boot and saved.
																								
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;  
		
	printk("Mprobe driver initialized.\n");
	
	

	return 0;
}
/* Driver Exit */
void __exit Mprobe_driver_exit(void)
{
	/* Release the major number */
	unregister_chrdev_region((Mprobe_dev_number), 1);

	/* Destroy device */
	device_destroy (Mprobe_dev_class, MKDEV(MAJOR(Mprobe_dev_number), 0));
	cdev_del(&Mprobe_devp->cdev);
	kfree(Mprobe_devp);
	
	/* Destroy driver_class */
	class_destroy(Mprobe_dev_class);
	printk("close is_registered val = %d\n",is_registered);
	printk("Mprobe driver removed.\n");
}

module_init(Mprobe_driver_init);
module_exit(Mprobe_driver_exit);
MODULE_LICENSE("GPL v2");