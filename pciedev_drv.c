#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/timer.h>

#include "pciedev_fnc.h"

MODULE_AUTHOR("Ludwig Petrosyan");
MODULE_DESCRIPTION("DESY AMC-PCIE board driver");
MODULE_VERSION("5.0.0");
MODULE_LICENSE("Dual BSD/GPL");

pciedev_cdev     *pciedev_cdev_m = 0;

static int        pciedev_open( struct inode *inode, struct file *filp );
static int        pciedev_release(struct inode *inode, struct file *filp);
static ssize_t pciedev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t pciedev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static long     pciedev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

struct file_operations pciedev_fops = {
    .owner                   =  THIS_MODULE,
    .read                     =  pciedev_read,
    .write                    =  pciedev_write,
    .unlocked_ioctl     =  pciedev_ioctl,
    .open                    =  pciedev_open,
    .release                =  pciedev_release,
};

static struct pci_device_id pciedev_ids[]  = {
    { PCIEDEV_VENDOR_ID, PCIEDEV_DEVICE_ID,
                   PCIEDEV_SUBVENDOR_ID, PCIEDEV_SUBDEVICE_ID, 0, 0, 0},
    { 0, }
};
MODULE_DEVICE_TABLE(pci, pciedev_ids);

/*
 * The top-half interrupt handler.
 */
static irqreturn_t pciedev_interrupt(int irq, void *dev_id UPKCOMPAT_IHARGPOS(regs) )
{
    struct pciedev_dev *pdev   = (pciedev_dev*)dev_id;
    struct module_dev *dev     = (module_dev *)(pdev->dev_str);

    //printk(KERN_ALERT "PCIEDEV_INTERRUPT:   DMA IRQ\n");
    dev->waitFlag = 1;
    //wake_up_interruptible(&(dev->waitDMA));
    wake_up(&(dev->waitDMA));
    return IRQ_HANDLED;
}

static int UPKCOMPAT_INIT pciedev_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    int result               = 0;
    module_dev  *module_dev_pp;
    pciedev_dev *module_pciedev;
    
    printk(KERN_ALERT "PCIEDEV_PROBE CALLED \n");
    result = pciedev_probe2_exp(dev, id, &pciedev_fops, pciedev_cdev_m,  &module_pciedev );
    printk(KERN_ALERT "PCIEDEV_PROBE_EXP CALLED\n");
    /*if board has created we will create our structure and pass it to pcedev_dev*/
    if(!result) {
        printk(KERN_ALERT "PCIEDEV_PROBE_EXP CREATING CURRENT STRUCTURE FOR BOARD %i\n", 
            module_pciedev->brd_num );
        module_dev_pp = kzalloc(sizeof(module_dev), GFP_KERNEL);
        if(!module_dev_pp){
                return -ENOMEM;
        }
        printk(KERN_ALERT "PCIEDEV_PROBE CALLED; CURRENT STRUCTURE CREATED \n");
        module_dev_pp->parent_dev  = module_pciedev;
        init_waitqueue_head(&module_dev_pp->waitDMA);
        pciedev_set_drvdata(module_pciedev, module_dev_pp);
        pciedev_setup_interrupt_exp(pciedev_interrupt, module_pciedev); 
    }
    return result;
}

static void UPKCOMPAT_EXIT pciedev_remove(struct pci_dev *dev)
{
    int result = 0;
    struct pciedev_dev *module_pciedev;     

    printk(KERN_ALERT "REMOVE CALLED\n");
    module_pciedev = pciedev_get_pciedata(dev);
    if( module_pciedev != NULL ) {
        module_dev  *module_dev_pp;
        int          slot_num = -1;
        int          brd_num = -1;

        slot_num      = module_pciedev->slot_num;
        brd_num       = module_pciedev->brd_num;
        module_dev_pp = pciedev_get_drvdata(module_pciedev);

        result = pciedev_remove2_exp(module_pciedev);
        printk(KERN_ALERT "PCIEDEV_REMOVE_EXP brd %i slot %i result=%d\n", 
            brd_num, slot_num, result);

        kfree(module_dev_pp);
    } else {
        printk(KERN_ALERT "REMOVE FAILED TO LOCATE MODULE\n");
    }
}

static struct pci_driver pci_pciedev_driver = {
    .name       = DEVNAME,
    .id_table   = pciedev_ids,
    .probe      = pciedev_probe,
    .remove     = UPKCOMPAT_EXIT_P(pciedev_remove),
};


static int pciedev_open( struct inode *inode, struct file *filp )
{
    int    result = 0;
    result = pciedev_open_exp( inode, filp );
    return result;
}

static int pciedev_release(struct inode *inode, struct file *filp)
{
    int result            = 0;
    result = pciedev_release_exp(inode, filp);
    return result;
} 

static ssize_t pciedev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t    retval         = 0;
    retval  = pciedev_read_exp(filp, buf, count, f_pos);
    return retval;
}

static ssize_t pciedev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t         retval = 0;
    retval = pciedev_write_exp(filp, buf, count, f_pos);
    return retval;
}

static long  pciedev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    
    if (_IOC_TYPE(cmd) == PCIEDOOCS_IOC){
        if (_IOC_NR(cmd) <= PCIEDOOCS_IOC_MAXNR && _IOC_NR(cmd) >= PCIEDOOCS_IOC_MINNR) {
            result = pciedev_ioctl_exp(filp, &cmd, &arg, pciedev_cdev_m);
        }else{
            result = pciedev_ioctl_dma(filp, &cmd, &arg, pciedev_cdev_m);
        }
    }else{
        return -ENOTTY;
    }
    return result;
}

static void __exit pciedev_cleanup_module(void)
{
    printk(KERN_NOTICE "PCIEDEV_CLEANUP_MODULE: PCI DRIVER UNREGISTERED\n");
    pci_unregister_driver(&pci_pciedev_driver);
    printk(KERN_NOTICE "PCIEDEV_CLEANUP_MODULE CALLED\n");
    upciedev_cleanup_module_exp(&pciedev_cdev_m);
}

static int __init pciedev_init_module(void)
{
    int   result  = 0;
    
    printk(KERN_ALERT "INIT: CALLING UPCIEDEV INIT \n");
    result = upciedev_init_module_exp(&pciedev_cdev_m, &pciedev_fops, DEVNAME);
    printk(KERN_ALERT "AFTER_INIT:REGISTERING PCI DRIVER\n");
    result = pci_register_driver(&pci_pciedev_driver);
    printk(KERN_ALERT "AFTER_INIT:REGISTERING PCI DRIVER RESUALT %d\n", result);
    return result; /* succeed */
}

module_init(pciedev_init_module);
module_exit(pciedev_cleanup_module);


