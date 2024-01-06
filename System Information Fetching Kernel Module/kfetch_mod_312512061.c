#include <asm/errno.h>
#include <linux/atomic.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/types.h> 
#include <linux/uaccess.h> 
#include <linux/version.h> 
#include <linux/time.h>
#include <linux/time_namespace.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/ktime.h>
#include <linux/mm.h>
#include <linux/sched.h> 
#include <linux/sched/signal.h>
#include <linux/smp.h>
#include <linux/utsname.h> 
#include <linux/timekeeping.h>
#include <linux/ktime.h>

#include <asm/errno.h> 

#define KFETCH_NUM_INFO 6
#define KFETCH_RELEASE   (1 << 0) //1shift0 ->int:1,bit position1 in bitmask.
#define KFETCH_NUM_CPUS  (1 << 1) //1shift1 ->int:2 ,bit position 2 in bitmask 
#define KFETCH_CPU_MODEL (1 << 2) //1shift2 ->int:4 , 
#define KFETCH_MEM       (1 << 3) //1shift3 ->int:8 , 
#define KFETCH_UPTIME    (1 << 4) //1shift4 ->int:16 , 
#define KFETCH_NUM_PROCS (1 << 5) //1shift5 ->int:32 , 
#define KFETCH_FULL_INFO ((1 << KFETCH_NUM_INFO) - 1); // 63. This value represents having all bits up to the position of KFETCH_NUM_INFO set to 1, creating a bitmask of KFETCH_NUM_INFO bits, all set to 1
#define SUCCESS 0 
#define DEVICE_NAME "kfetch" /* Dev name as it appears in /proc/devices   */ 
#define BUF_SIZE 1024 /* Max length of the message from the device */ 


//prototype The file_operations structure is defined in include/linux/fs.h
static int kfetch_init(void);
static void kfetch_exit(void);
static ssize_t kfetch_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t kfetch_write(struct file *, const char __user *, size_t, loff_t *);
static int kfetch_open(struct inode *, struct file *);
static int kfetch_release(struct inode *, struct file *);
//end of prototype

/* Global variables */
static int major; /* major number assigned to our device driver */ 
static char kfetch_buf[BUF_SIZE];
static bool open_device = false;
static int kfetch_mask = 0;

static struct class *cls; 
//device operation
const static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};


//
static int kfetch_init(void) {
    // this part is in  The Linux Kernel Module Programming Guide ch6.5
    major = register_chrdev(0, DEVICE_NAME, &kfetch_ops ); 
 
    if (major < 0) { 
        pr_alert("Registering char device failed with %d\n", major); 
        return major; 
    } 
    kfetch_mask = ((1 << KFETCH_NUM_INFO) - 1);//63
    pr_info("I was assigned major number %d.\n", major); 
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
    cls = class_create(DEVICE_NAME); 
#else 
    cls = class_create(THIS_MODULE, DEVICE_NAME);  //this is our version
#endif 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 
 
    pr_info("Device created on /dev/%s\n", DEVICE_NAME); 
 
    return SUCCESS; 
}

static void kfetch_exit(void) {
    // this part is in  The Linux Kernel Module Programming Guide ch6.5
    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls); 
 
    /* Unregister the device */ 
    unregister_chrdev(major, DEVICE_NAME); 

}
/* Methods */ 
 


//open

static int kfetch_open(struct inode *inode, struct file *file) 
{ 
  // this part is in  The Linux Kernel Module Programming Guide ch6.5
    if (open_device)
        return -EBUSY;
    open_device = true; 
    try_module_get(THIS_MODULE); 
 
    return SUCCESS; 
} 

//release
/* Called when a process closes the device file. */ 
static int kfetch_release(struct inode *inode, struct file *file) 
{ 
  // this part is in  The Linux Kernel Module Programming Guide ch6.5
    /* We're now ready for our next caller */ 
    open_device = false;
  
    /* Decrement the usage count, or else once you opened the file, you will 
     * never get rid of the module. 
     */ 
    module_put(THIS_MODULE); 
 
    return SUCCESS; 
} 




//read
static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    // todo
    char *logo[8] = {
    "                    ",
    "         .-.        ",
    "        (.. |       ",
    "        <>  |       ",
    "       / --- \\      ",
    "      ( |   | |     ",
    "    |\\\\_)___/\\)/\\   ",
    "   <__)------(__/   ",
    };

    char info_list[8][64];
    char *hostname;
    char split_line[30];
    char *kernel;
    char *cpu_info;
    int cpu_family;
    int cpu_activate;
    int memory_total;
    int memory_free;
    int procs =0;
    int time_min;
    char buf[64]={0};
    int toshow[8] = {1,1,0,0,0,0,0,0}; //1:show 0:not show
    //host name
    hostname = utsname()->nodename;
    strcpy(info_list[0], hostname);


    //split line
    int hostname_length = strlen(hostname);
    int l;
    for (;l<hostname_length;l++)
    	split_line[l] = '-';
    split_line[l] = '\0';
    strcpy(info_list[1], split_line);
    //kernel
    kernel = utsname()->release;
    sprintf(buf, "Kernal:  %s", kernel);
    strcpy(info_list[2], buf);
    
    //CPU
    struct cpuinfo_x86 *c = &boot_cpu_data;
    cpu_info = c->x86_model_id;
    sprintf(buf, "CPU:     %s", cpu_info);
    strcpy(info_list[3], buf);
    
    //CPUs
    cpu_family = c->x86;
    cpu_activate = num_online_cpus();
    sprintf(buf, "CPUs:    %d / %d", cpu_activate, cpu_family);
    strcpy(info_list[4], buf);
    
    //Mem
    struct sysinfo info;
    si_meminfo(&info);
    memory_total = info.totalram * info.mem_unit/1024/1024;
    memory_free =  info.freeram * info.mem_unit/1024/1024;
    sprintf(buf, "Mem:     %d MB/ %d MB", memory_free, memory_total);
    strcpy(info_list[5], buf);
    
    //proc
    struct task_struct *task;
    for_each_process(task) {
        procs++;
    }
    sprintf(buf, "Procs:   %d ", procs);
    strcpy(info_list[6], buf);
    
    //uptime
    u64 boot_time_ns, uptime_seconds;
    boot_time_ns = ktime_get_raw_ns();
    uptime_seconds = boot_time_ns / NSEC_PER_SEC;
    time_min = uptime_seconds/60;
    sprintf(buf, "Uptime:  %d mins", time_min);
    strcpy(info_list[7], buf);
    
    
    //printk(KERN_INFO "kfetch_mask: %d \n", kfetch_mask);
    
    // to check which messages should be shown 
    if (kfetch_mask & KFETCH_RELEASE) {
        toshow[2] = 1;
    }
    if (kfetch_mask & KFETCH_CPU_MODEL) {
        toshow[3] = 1;
    }
    if (kfetch_mask & KFETCH_NUM_CPUS) {
        toshow[4] = 1;
    }
    if (kfetch_mask & KFETCH_MEM) {
        toshow[5] = 1;
    }
    if (kfetch_mask & KFETCH_NUM_PROCS) {
        toshow[6] = 1;
    }
    if (kfetch_mask & KFETCH_UPTIME) {
        toshow[7] = 1;
    }

    
    
    strcpy(kfetch_buf, "");
    for (int i = 0; i < 8; i++) {
        strcat(kfetch_buf, logo[i]);
        if (toshow[i] == 1)
        {
            strcat(kfetch_buf, info_list[i]);
        }
        
        
    	strcat(kfetch_buf, "\n");
        
    }


    
    if (copy_to_user(buffer, kfetch_buf, sizeof(kfetch_buf))) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    return sizeof(kfetch_buf);
    /* cleaning up */
}

//write
static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    //Declares an integer variable mask_info to store the bitmask 	 information received from the user-space buffer.
    int mask_info;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }
    
    /* setting the information mask */
    kfetch_mask = mask_info;
    return SUCCESS;
}


module_init(kfetch_init);
module_exit(kfetch_exit);

MODULE_LICENSE("GPL");
