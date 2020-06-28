/*
 * 内核驱动程序
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "shared.h"

int shared_major = SHARED_MAJOR;    //主设备号
int shared_minor = 0;   //第一个次设备号
int shared_devices_num = DEVICES_NUM;   //需要初始化的设备数
struct shared_dev *shared_device;   //只想第一个结构的指针

module_param(shared_major, int, S_IRUGO);
module_param(shared_minor, int, S_IRUGO);

/*
 * 释放占用的内存
 * dev：需要释放的设备
 */
int shared_trim(struct shared_dev *dev){
    if (dev){
        if (dev->data){
            kfree(dev->data);
        }
        dev->data = NULL;
        dev->size = 0;
    }

    return 0;
}

/*
 * 打开文件
 * inode：inode节点
 * filp：file指针
 */
int shared_open(struct inode *inode, struct file *filp){
    struct shared_dev *dev;

    dev = container_of(inode->i_cdev, struct shared_dev, cdev); //获取包含cdev结构的shared_dev结构
    filp->private_data = dev;

    if ((filp->f_flags & O_ACCMODE) == O_WRONLY){   //如果不可打开
        if (down_interruptible(&dev->sem)){ //进入临界区
            return -ERESTARTSYS;
        }
        shared_trim(dev);
        up(&dev->sem);
    }

    return 0;
}

/*
 * 释放文件，暂时用不上，只用于占位
 */
int shared_release(struct inode *inode, struct file *filp){
    return 0;
}


/*
 * 读函数，返回成功读取的字节数
 * filp：file指针
 * buf：用户空间的缓冲区
 * count：数量
 * f_pos：偏移量
 */
ssize_t shared_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    struct shared_dev *dev = filp->private_data;
    ssize_t retval = 0;

    if (down_interruptible(&dev->sem)){ //进入临界区
        return -ERESTARTSYS;
    }
    if (*f_pos >= dev->size){   //到达文件尾
        goto out;
    }
    if (*f_pos + count > dev->size){    //截断
        count = dev->size - *f_pos;
    }
    if (!dev->data){    //无可读数据
        goto out;
    }
    if (copy_to_user(buf, dev->data + *f_pos, (unsigned long)count)){   //将数据复制到用户空间，如果失败则
        retval = -EFAULT;
        goto out;
    }

    //更新节点信息
    *f_pos += count;
    retval = count;

    out:
    up(&dev->sem);  //释放信号量
    return retval;
}

/*
 * 写函数，返回成功写入的字节数
 * filp：file指针
 * buf：用户空间的缓冲区
 * count：数量
 * f_pos：偏移量
 */
ssize_t shared_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    struct shared_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;

    if (down_interruptible(&dev->sem)){ //进入临界区
        return -ERESTARTSYS;
    }
    shared_trim(dev);   //先释放原来的内容

    if (!dev->data){    //初始化数据
        dev->data = kmalloc(SHARED_BUFFER_SIZE, GFP_KERNEL);
        if (!dev->data){
            goto out;
        }

        memset(dev->data, 0, SHARED_BUFFER_SIZE);   //初始化为0
    }

    if (count > SHARED_BUFFER_SIZE - dev->size){    //截断
        count = SHARED_BUFFER_SIZE - dev->size;
    }

    if (copy_from_user(dev->data + dev->size, buf, count)){ //从用户空间复制数据
        retval = -EFAULT;
        goto out;
    }

    //更新信息
    dev->size += count;
    retval = count;

    out:
    up(&dev->sem);  //释放信号量
    return retval;
}

/*
 * 改变偏移量
 */
loff_t shared_llseek(struct file *filp, loff_t off, int whence){
    struct shared_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence){ //根据不同模式更新偏移量
        case 0:
            newpos = off;
            break;
        case 1:
            newpos = filp->f_pos + off;
            break;
        case 2:
            newpos = dev->size + off;
            break;
        default:
            return -EINVAL;
    }

    if (newpos < 0){
        return -EINVAL;
    }

    filp->f_pos = newpos;
    return newpos;
}

struct file_operations shared_fops = {  //文件操作结构
        .owner = THIS_MODULE,
        .llseek = shared_llseek,
        .read = shared_read,
        .write = shared_write,
        .open = shared_open,
        .release = shared_release,
};

/*
 * 退出函数
 */
void shared_cleanup_module(void){
    int i;
    for(i = 0; i < shared_devices_num; i++){
        dev_t devno = MKDEV(shared_major, shared_minor + i);    //cdev设备号

        if (shared_device){
            shared_trim(shared_device); //释放内存
            cdev_del(&shared_device->cdev); //删除cdev设备
        }
        unregister_chrdev_region(devno, 1); //注销一个cdev设备
    }
    kfree(shared_device);   //释放指针占用的内存
}

/*
 * 初始化单个cdev
 */
static void shared_setup_cdev(struct shared_dev *dev, int index){
    int err;
    int devno = MKDEV(shared_major, shared_minor + index);

    cdev_init(&dev->cdev, &shared_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &shared_fops;
    err = cdev_add(&dev->cdev, devno, 1);

    if (err){
        pr_info("Error %d adding shared", err);
    }

}

/*
 * 初始化函数
 */
static int __init shared_init_module(void){
    int result;
    int i;
    dev_t dev = 0;

    if (shared_major){  //静态分配
        dev = MKDEV(shared_major, shared_minor);
        result = register_chrdev_region(dev, 1, "shared");
    } else{ //动态分配
        result = alloc_chrdev_region(&dev, shared_minor, 1, "shared");
        shared_major = MAJOR(dev);
    }

    if (result < 0){
        pr_info("shared: can't get major %d\n", shared_major);
        return result;
    }

    //分配设备内存并初始化为0
    shared_device = kmalloc(shared_devices_num * sizeof(struct shared_dev), GFP_KERNEL);
    if (!shared_device){
        result = -ENOMEM;
        goto fail;
    }
    memset(shared_device, 0, shared_devices_num * sizeof(struct shared_dev));

    for(i = 0; i < shared_devices_num; i++){    //初始化shared_devices_num个cdev
        sema_init(&shared_device[i].sem, 1);
        shared_setup_cdev(&shared_device[i], i);
        pr_info("created %d %d\n", shared_major, shared_minor + i);
    }
    return 0;

    fail:
    shared_cleanup_module();
    return result;
}


module_init(shared_init_module);
module_exit(shared_cleanup_module);

MODULE_LICENSE("GPL");