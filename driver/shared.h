#ifndef _SHARED_H
#define _SHARED_H

#define SHARED_MAJOR 666    //主设备号，为0则动态分配
#define SHARED_BUFFER_SIZE PAGE_SIZE    //将缓冲区设为页大小
#define DEVICES_NUM 10  //最大设备数

struct shared_dev {
    char *data;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

int shared_trim(struct shared_dev *dev);
int shared_open(struct inode *inode, struct file *filp);
int shared_release(struct inode *inode, struct file *filp);
ssize_t shared_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t shared_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
loff_t shared_llseek(struct file *filp, loff_t off, int whence);
void shared_cleanup_module(void);
static void shared_setup_cdev(struct shared_dev *dev, int index);
static int __init shared_init_module(void);

#endif