 
/**************************************************************
* Class:  CSC-415-02 Fall 2020
* Name: Em Powers
* Student ID: 92073439
* Project: Assignment 6 - Device Driver
*
* File: device_driver_main.c
*
* device_driver_main is a device driver representing a simple calculator interface
* written in c. It provides functionality for read, write, open, release, and ioctl
* to perform simple calculations in kernel space. It maps to /dev/sampledriver and
* includes a tester program for functionality.
*
**************************************************************/
 
#include <linux/init.h>           // Include init macros for __init functionality
#include <linux/module.h>         // Loads this LKM to the kernel
#include <linux/device.h>         // Support for the driver model
#include <linux/kernel.h>         // Types and functions for the kernel
#include <linux/fs.h>             // To access Linux file system support
#include <linux/uaccess.h>        // Copy to user functionality 
#include <linux/cdev.h>           // Basic registration interface for character devices
#include <linux/types.h>          // Includes linux-supported types 
#include <linux/mutex.h>          // Functionality for mutex

#define  DEVICE_NAME "sampledriver"    // Device will be at /dev/sampledriver address
#define  CLASS_NAME  "dri"        // Will indicate device class with this definition

static DEFINE_MUTEX(devchar_mutex); // defines the mutex functionality for our user program
                                    // that will result in a semaphore with value 1 if unlocked
                                    // or 0 if locked
 
MODULE_LICENSE("GPL");            // Will be the license for this driver
MODULE_AUTHOR("Em Powers");    // It's me!
MODULE_DESCRIPTION("Linux kernel driver for CSC415");  // Acts as as a description for the driver
MODULE_VERSION("0.1");            // Informs users current version of the driver 
 
static int    majorNumber;                  // Major number used to indicate device
static int    numberOpens = 0;              // Number of times the device has opened
static struct class*  sampledevClass  = NULL; // Struct pointer to the sample class
static struct device* sampledevDevice = NULL; // Struct pointer to the sample device
int32_t value = 0; // Final value of our calculation
int32_t leftSide = 0; // Left side of the mathematic operation
int32_t rightSide = 0; // Right side of the mathematic operation
int32_t operator = 0; // The operator, represented as a number between 1 and 4


#define WR_VALUE _IOW('a', 'a', int32_t*) // Functionality for IOW, necessary for ioctl
#define RD_VALUE _IOR('a', 'b', int32_t*) // Functionality for IOR, necessary for ioctl

// Prototype functions for the device driver, needed for their implementation later

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);
 
/** 
 * 
 * As devices are represented as file structures in the kernel, we will need to map our 
 * functionality to the file operations from the functions we have implemented. 
 *
 */

static struct file_operations fops =
{
   .owner = THIS_MODULE,
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .unlocked_ioctl = dev_ioctl,
   .release = dev_release,
};
 
/** 
 * 
 *  devchar_init 
 *  This function serves to initialize our device driver. It is protected to within this 
 *  file, and the __init macro indicates that this function is only used at initialization time
 *  so memory is freed. It contains functionality for registering the character device,
 *  as well as creating the driver class and device. The user is informed about successes.
 * 
 *  @return returns 0 if successful
 */

static int __init devchar_init(void){
   
   printk(KERN_INFO "SampleDevice: Initializing the Sample Device LKM...\n");
 
   // This dynamically allocates a major number for our device to track it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

   // Error handling if we failed to register a major number
   if (majorNumber<0){
      printk(KERN_ALERT "SampleDevice failed to register a major number\n");
      return majorNumber;
   }

   printk(KERN_INFO "SampleDevice: registered correctly with major number %d\n", majorNumber);
 
   // We need to be able to register the device class, so this handles creation

   sampledevClass = class_create(THIS_MODULE, CLASS_NAME);

   // Checks for error handling here.

   if (IS_ERR(sampledevClass)){         
      // Make sure to unregister the device if making the class fails       
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");

      // Since the sampledevClass is a pointer, we need to return a pointer error
      return PTR_ERR(sampledevClass);          
   }

   printk(KERN_INFO "SampleDevice: device class registered correctly\n");
 
   // Here, we need to register the driver for our device

   sampledevDevice = device_create(sampledevClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);

   // Check for errors and destroy class/unregister device if so
   if (IS_ERR(sampledevDevice)){               
      class_destroy(sampledevClass);           
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(sampledevDevice);
   } 
   
   // We've initialized the device!
   printk(KERN_INFO "SampleDevice: device class created correctly\n");
   // This will serve to initialize the mutex lock dynamically 
   mutex_init(&devchar_mutex);

   return 0;
}

/** 
 *  
 *  We need to make use for the module_init() and module_exit() macros from init.h to
 *  initialize our driver and call the cleanup function.
 *
 */

module_init(devchar_init);

 /** 
 * 
 *  devchar_exit
 *
 *  Similarly to the init function, this acts to deregister all elements of the device driver -
 *  the mutex lock to protect from race conditions, the device, the class, and the major number.
 * 
 */

static void __exit devchar_exit(void){
   mutex_destroy(&devchar_mutex);
   device_destroy(sampledevClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(sampledevClass);                          // unregister the device class
   class_destroy(sampledevClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "SampleDevice: Goodbye from the LKM!\n");
}

 /** 
 * 
 *  dev_open
 *
 *  This function will attempt to check the mutex for race conditions. If there are none,
 *  the user is alerted that the device has been opened and how many times it has been opened.
 * 
 */

static int dev_open(struct inode *inodep, struct file *filep){

   // We will try and acquire the mutex here. It returns 1 if successful or 0 if not
   // and will return a busy status if the device is already in use. 

   if(!mutex_trylock(&devchar_mutex)){
      printk(KERN_ALERT "SampleDevice: Device is already in use by another process.");
      return -EBUSY;
   }

   // If not, we will increment the number of times the device has been opened and present this
   // information to the user. 

   numberOpens++;
   printk(KERN_INFO "SampleDevice: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}
 
  /** 
 * 
 *  dev_read
 *  @param filep
 *  @param buffer 
 *  @param len 
 *  @param offset
 *
 *  This function is called when the device is being read from user space, and sends data from the
 *  device space to the user space. In this case, it will return whatever the result of the user
 *  calculation is. 
 * 
 */

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;

   // copy_to_user is the function that copies data from device to user space

   error_count = copy_to_user((int32_t*) buffer, &value, sizeof(value));

   // If there are no errors, we can safely print a message indicating the result of our read function
   if (error_count==0){            
      printk(KERN_INFO "SampleDevice: Sent our result of size %ld to the user\n", sizeof(value));
   } else {
      printk(KERN_INFO "SampleDevice: Failed to send our result %d to the user\n", error_count);

      // Bad address error sent to user
      return -EFAULT;              
   }
   return 0;
}

 /** 
 * 
 *  dev_write
 *  @param filep
 *  @param buffer 
 *  @param len 
 *  @param offset
 * 
 *  This function will attempt to write from user space to device space whatever the user
 *  passes in. With this program, write attempts to write a number to user space.
 * 
 */

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   static int counter = 0;

   // We use a counter to determine if the parameter is the operator, the left hand side,
   // or the right-hand side
   if(counter == 0){
      copy_from_user(&operator, (int32_t*)buffer, sizeof(value));
   }

   if(counter == 1){
      copy_from_user(&leftSide, (int32_t*)buffer, sizeof(value));
   }

   if(counter == 2){
      copy_from_user(&rightSide, (int32_t*)buffer, sizeof(value));
   }

   // We must increment the counter every time ioctl is written

   counter = counter + 1;
   
 // We must reset the counter after the user has inputted all three values in the equation

   if(counter == 3){
      counter = 0;
   }

   printk(KERN_INFO "VALUE = %d\n",value);

   return 0;
}

/** 
 * 
 *  dev_ioctl
 *  @param filep
 *  @param cmd
 *  @param arg
 * 
 *  This function provides calculator functionality based off of what the user has written to
 *  device space. 
 * 
 */

static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg){
   
   switch(cmd){
      // RD_VALUE is the user indicator as to what ioctl should do
      // more cases can be added if more functionality is wished for
      case RD_VALUE:

         // In this case, RD_VALUE will perform an operation based off of 
         // user specifications

         if(operator == 1){
            value = leftSide + rightSide;
         }
         else if(operator == 2){
            value = leftSide - rightSide;
         }
         else if(operator == 3){
            value = leftSide * rightSide;
         }
         else if(operator == 4){
            value = leftSide / rightSide;
         }else{
            break;
         }
         break;
      default:
         break;
   }
   return 0;
}
 
/** 
 * 
 *  dev_release
 *  @param inodep
 *  @param filep
 * 
 *  This function will be referenced when the device is closed or released by the user program
 *  and informs the user that it has been released.
 * 
 */
 
static int dev_release(struct inode *inodep, struct file *filep){

   // We need to unlock the mutex as race conditions are no longer concerning
   mutex_unlock(&devchar_mutex);
   printk(KERN_INFO "SampleDevice: Device has been successfully closed. \n");
   return 0;
}

/** 
 *  
 *  Much like module_init(), we need to use module_exit to indicate that our
 *  device program is exited out of now. 
 *
 */
 
module_exit(devchar_exit);
