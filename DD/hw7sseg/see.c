//character device driver
//7 segment module
//gpio driver

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

#define BUF_SIZE 10
#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "segment"
#define A 16
#define B 12
#define C 13
#define D 19
#define E 26
#define F 20
#define G 21
#define DP 6

struct cdev segment_cdev;
static char msg[BUF_SIZE] = {'\0'};
static int gpio_list[] = {A,B,C,D,E,F,G,DP};
static int number_array[][8] = 
{
//   a b c d e f g dp 
	{0,0,0,0,0,0,1,1}, //0
	{1,0,0,1,1,1,1,1}, //1
	{0,0,1,0,0,1,0,1}, //2
	{0,0,0,0,1,1,0,1}, //3
	{1,0,0,1,1,0,0,1}, //4
	{0,1,0,0,1,0,0,1}, //5
	{0,1,0,0,0,0,0,1}, //6
	{0,0,0,1,1,1,1,1}, //7
	{0,0,0,0,0,0,0,1}, //8
	{0,0,0,0,1,0,0,1}, //9
	{0,0,0,0,0,0,0,0}, //allOne
	{1,1,1,1,1,1,1,1}, //allZero
};
static int segment_open(struct inode *inod, struct file *fil)
{
	try_module_get(THIS_MODULE);
	printk(KERN_INFO "segment_open called\n");
	return 0;
}

static int segment_close(struct inode *inod, struct file *fil)
{
	module_put(THIS_MODULE);
	printk(KERN_INFO "segment_close called\n");
	return 0;
}

static ssize_t segment_read(struct file *fil, char *buff, size_t len, loff_t *off)
{
	return 0;
}
static ssize_t segment_write(struct file *fil, const char *buff, size_t len, loff_t *off)
{
	int count;
	int number;
	int i;
	memset(msg, 0, BUF_SIZE);
	count = copy_from_user(msg, buff, len);
	number = simple_strtol(msg, NULL, BUF_SIZE);
	printk(KERN_INFO "you wrote a number : %d\n", number);
	
	//  a ~ dp
	for(i = 0; i<8; i++)
	{
		gpio_set_value(gpio_list[i], number_array[number][i]);
	}

	return count;
}

static struct file_operations segment_fops=
{
	.owner = THIS_MODULE,
	.read = segment_read,
	.write = segment_write,
	.open = segment_open,
	.release = segment_close,
};


static int __init initModule(void)
{
	dev_t devno;
	int err;
	int count;
	int i;
	char buff[2];
	printk("Called initModule()\n");

	devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	register_chrdev_region(devno, 1, GPIO_DEVICE);

	cdev_init(&segment_cdev, &segment_fops);
	segment_cdev.owner = THIS_MODULE;
	count = 1;

	err = cdev_add(&segment_cdev, devno, count);
	if(err < 0)
	{
		printk(KERN_INFO "Error: cdev_add()");
		return -1;
	}

	printk(KERN_INFO "'sudo mknod /dev/%s c %d %d'\n", GPIO_DEVICE, GPIO_MAJOR, GPIO_MINOR);
	printk(KERN_INFO "'sudo chmod 666 /dev/%s'\n", GPIO_DEVICE);

	for(i = 0; i< 8; i++)
	{
		sprintf(buff, "%d", gpio_list[i]);
		err = gpio_request(gpio_list[i], buff);
		if(err == -EBUSY)
		{
			printk(KERN_INFO "Error gpio_request : %s\n", buff);
			return -1;
		}
		gpio_direction_output(gpio_list[i], 1);
	}
	return 0;
}

static void __exit cleanupModule(void)
{
	dev_t devno;
	int i;
	devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
	for(i = 0; i<8; i++)
	{
		gpio_set_value(gpio_list[i], 1);
		gpio_free(gpio_list[i]);
	}
	unregister_chrdev_region(devno, 1);
	cdev_del(&segment_cdev);
	printk(KERN_INFO "cleanupModule() called\n");
}

module_init(initModule);
module_exit(cleanupModule);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO Driver Module");
MODULE_AUTHOR("WJ PARK");
