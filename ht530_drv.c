/* Basic driver example to show skelton methods for several file operations.
 references: http://lxr.free-electrons.com/source/tools/include/linux/list.h#L713
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
#include <linux/errno.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
#include <linux/hashtable.h>
#include "ht530_ioctl.h"
#define DEVICE_NAME                 "ht530"  // device name of the hashtable


int set_retval;
int KEY;
 int errno;
typedef struct ht_obj_node{
	
	ht_obj_t new_ht_obj;
	struct hlist_node my_next;
}ht_obj_node, *Pht_obj_node;
	
DEFINE_HASHTABLE(ht530_tbl, 7);  

/* per device structure */
 struct ht530_dev {
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	Pht_obj_node Pht_obj_node_new;
} *ht530_devp;

static dev_t ht530_dev_number;      /* Allotted device number */
struct class *ht530_dev_class;          /* Tie with the device model */
static struct device *ht530_dev_device;

static char *user_name = "CSE530 Assignment1";

module_param(user_name,charp,0000);	//to get parameter from load.sh script to greet the user

/*
* Open ht530 driver
*/
int ht530_driver_open(struct inode *inode, struct file *file)
{
	struct ht530_dev *ht530_devp;

	/* Get the per-device structure that contains this cdev */
	ht530_devp = container_of(inode->i_cdev, struct ht530_dev, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = ht530_devp;
	printk("\n%s is opening \n", ht530_devp->name);
	
	
	return 0;
}

long ht530_driver_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int count=0;
	Pht_obj_node temp=NULL;
	void *pIoBuffer = NULL;
	
	if (!(pIoBuffer = kmalloc(_IOC_SIZE(ioctl_num), GFP_KERNEL)))
	{
		printk("ht530_dev_error: unable to allocate io buffer memory for ioctl \n");
		return -ENOMEM;
	}
	if(copy_from_user(pIoBuffer,(unsigned long *)(ioctl_param), _IOC_SIZE(ioctl_num)))
	{
		printk("ht530_dev_error: unable to copy data from user space\n");
		kfree(pIoBuffer);
		return -ENOMEM;
	}

	switch(ioctl_num)
	{
	
	case HT_530_READ_KEY:
	KEY = *((int*)pIoBuffer);  
	break;

	case DUMP_IOCTL:
	printk("\n DUMP_IOCTL call\n");
	((Pdump_arg)pIoBuffer)->RetVal = 0;
	hlist_for_each_entry( temp,&ht530_tbl[((Pdump_arg)pIoBuffer)->in.in_n ], my_next)
	{
		if(count < 8)
		{printk( "BUCKET %d:   ioctl_dump KEY = %d   ioctl_dump DATA =%d\n",((Pdump_arg)pIoBuffer)->in.in_n, temp->new_ht_obj.key,temp->new_ht_obj.data);
		((Pdump_arg)pIoBuffer)->out.out_object_array[count] = temp->new_ht_obj;
		count++;}
	}	
	((Pdump_arg)pIoBuffer)->in.in_n = count; 
	if (copy_to_user((unsigned long *)(ioctl_param), (unsigned long *) pIoBuffer, _IOC_SIZE(ioctl_num))) 
	{
		printk(" in dump_ioctl unable to copy data to user space\n");
		kfree(pIoBuffer);
		
	}
	break;
	kfree(pIoBuffer);
	}	
	return 0;
}

	
/*
 * Release ht530 driver
 */
int ht530_driver_release(struct inode *inode, struct file *file)
{
	struct ht530_dev *ht530_devp = file->private_data;
	
	printk("\n%s is closing\n", ht530_devp->name);
	
	return 0;
}

/*
 * Write to ht530 driver
 */
ssize_t ht530_driver_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{

	int buk;
	int flag = 0;
	int flag2 = 0;
	struct ht530_dev *ht530_devp = file->private_data;
	Pht_obj_node temp=NULL;
	Pht_obj_node pnew_ht_obj_node = NULL ;
	Pht_obj_node ppnew_ht_obj_node = NULL ;
if(!(ppnew_ht_obj_node = kmalloc(sizeof(struct ht_obj_node), GFP_KERNEL)))
{		printk("Bad Kmalloc\n");
		 return -ENOMEM;
}	
	memset(ppnew_ht_obj_node, 0, sizeof (ht_obj_node));
	
		if(copy_from_user(&ppnew_ht_obj_node->new_ht_obj, buf, count)) 
		return -EFAULT;
		

		ht530_devp->Pht_obj_node_new = ppnew_ht_obj_node;
		pnew_ht_obj_node= ht530_devp->Pht_obj_node_new;		
		
	printk("Performing WRITE operation with:  KEY= %d,  DATA= %d\n", pnew_ht_obj_node->new_ht_obj.key, pnew_ht_obj_node->new_ht_obj.data);	
		if(hash_empty(ht530_tbl))

		{
			hash_add(ht530_tbl, &pnew_ht_obj_node->my_next, pnew_ht_obj_node->new_ht_obj.key);
			flag2 =1;
		}
		else
		{
		hash_for_each(ht530_tbl, buk, temp, my_next){           
	    if((temp->new_ht_obj.key == pnew_ht_obj_node->new_ht_obj.key) && (pnew_ht_obj_node->new_ht_obj.data !=0) ){
			hash_del(&temp->my_next);
			flag = 1;
			hash_add(ht530_tbl, &pnew_ht_obj_node->my_next, pnew_ht_obj_node->new_ht_obj.key);
		}
		else if((pnew_ht_obj_node->new_ht_obj.data ==0)&&(temp->new_ht_obj.key == pnew_ht_obj_node->new_ht_obj.key)){
			hash_del(&temp->my_next);
			flag = 1;
			
		}
		}
		}
		if((flag2== 0) && (flag == 0) && (pnew_ht_obj_node->new_ht_obj.data !=0))
		 hash_add(ht530_tbl, &pnew_ht_obj_node->my_next, pnew_ht_obj_node->new_ht_obj.key);

return 0;
}
/*
 * Read to ht530 driver
 */
ssize_t ht530_driver_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)   //assumimg we know the key that is stored in read_key (a global variable)
{	int buk;
	int size_data =0;
	int read_data = 0;
	Pht_obj_node temp=NULL;
	 hash_for_each(ht530_tbl, buk, temp, my_next){ 
	  if(temp->new_ht_obj.key == KEY)
		read_data = temp->new_ht_obj.data;
	  }
	  if(read_data != 0)
	  { size_data = sizeof(int);
		printk("\nPerforming READ operation with KEY= %d Data= %d\n",KEY, read_data);
		if (copy_to_user((int*)buf, (int*)&read_data, size_data)) 
        	{	printk("unable to copy to  the user");
			return -EFAULT;
		}
	  }
	  else
	  {
		printk("is invalid \n");
		errno = EINVAL;
		return -1;
	  }
	
	return size_data;

}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations ht530_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= ht530_driver_open,        /* Open method */
    .release	= ht530_driver_release,     /* Release method */
    .write		= ht530_driver_write,       /* Write method */
    .read		= ht530_driver_read,        /* Read method */
   .unlocked_ioctl	= ht530_driver_ioctl,
};

/*
 * Driver Initialization
 */
int __init ht530_driver_init(void)
{
	int ret;
	int time_since_boot;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&ht530_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	ht530_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	ht530_devp = kmalloc(sizeof(struct ht530_dev), GFP_KERNEL);
		
	if (!ht530_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(ht530_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&ht530_devp->cdev, &ht530_fops);
	ht530_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&ht530_devp->cdev, (ht530_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	ht530_dev_device = device_create(ht530_dev_class, NULL, MKDEV(MAJOR(ht530_dev_number), 0), NULL, DEVICE_NAME);		

	
	time_since_boot=(jiffies-INITIAL_JIFFIES)/HZ;//since on some systems jiffies is a very huge uninitialized value at boot and saved.
	

	printk("ht530 driver initialized.\n");
	
	
	return 0;
}
/* Driver Exit */
void __exit ht530_driver_exit(void)
{
	int buk;
	Pht_obj_node temp=NULL;
	hash_for_each(ht530_tbl, buk, temp, my_next){           
			hash_del(&temp->my_next);}
	unregister_chrdev_region((ht530_dev_number), 1);

	/* Destroy device */
	device_destroy (ht530_dev_class, MKDEV(MAJOR(ht530_dev_number), 0));
	cdev_del(&ht530_devp->cdev);
	kfree(ht530_devp);
	
	/* Destroy driver_class */
	class_destroy(ht530_dev_class);

	printk("ht530 driver removed.\n");
}

module_init(ht530_driver_init);
module_exit(ht530_driver_exit);
MODULE_LICENSE("GPL v2");
