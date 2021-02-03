/**
*Copyright 2016-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
*
*This file is part of PCIEDEV driver.
*
*PCIEDEV is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*PCIEDEV is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with PCIEDEV.  If not, see <http://www.gnu.org/licenses/>.
**/


/*
*	Author: Ludwig Petrosyan (Email: ludwig.petrosyan@desy.de)
*/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/mm.h>

#include "pciedev_fnc.h"

MODULE_AUTHOR("Ludwig Petrosyan");
MODULE_DESCRIPTION("DESY AMC-PCIE board driver");
MODULE_VERSION("7.0.0");
MODULE_LICENSE("Dual BSD/GPL");

pciedev_cdev     *pciedev_cdev_m = 0;
module_dev       *module_dev_p[PCIEDEV_NR_DEVS];
module_dev       *module_dev_pp;

extern struct upciedev_base_dev *p_base_upciedev_dev;

static int        pciedev_open( struct inode *inode, struct file *filp );
static int        pciedev_release(struct inode *inode, struct file *filp);
static ssize_t pciedev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t pciedev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static long    pciedev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int      pciedev_remap_mmap(struct file *filp, struct vm_area_struct *vma);

struct file_operations pciedev_fops = {
	.owner                   =  THIS_MODULE,
	.read                     =  pciedev_read,
	.write                    =  pciedev_write,
	.unlocked_ioctl     =  pciedev_ioctl,
	.open                    =  pciedev_open,
	.release                =  pciedev_release,
	.mmap                 = pciedev_remap_mmap,
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
#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
static irqreturn_t pciedev_interrupt(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t pciedev_interrupt(int irq, void *dev_id)
#endif
{
    uint32_t intreg = 0;
    
    struct pciedev_dev *pdev   = (pciedev_dev*)dev_id;
    struct module_dev *dev     = (module_dev *)(pdev->dev_str);
    
    //printk(KERN_ALERT "PCIEDEV_INTERRUPT:   DMA IRQ\n");
    dev->waitFlag = 1;
    //wake_up_interruptible(&(dev->waitDMA));
    wake_up(&(dev->waitDMA));
    return IRQ_HANDLED;
}

/*
 * The top-half interrupt handler. for MSI Buno code testing
 */
#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
static irqreturn_t pciedev_interrupt2(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t pciedev_interrupt2(int irq, void *dev_id)
#endif
{
    uint32_t intreg = 0;
    
    struct pciedev_dev *pdev   = (pciedev_dev*)dev_id;
    struct module_dev *dev     = (module_dev *)(pdev->dev_str);
    
    //printk(KERN_ALERT "PCIEDEV_INTERRUPT:   MSI  IRQ 2\n");
    return IRQ_HANDLED;
}

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
static irqreturn_t pciedev_interrupt3(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t pciedev_interrupt3(int irq, void *dev_id)
#endif
{
    uint32_t intreg = 0;
    
    struct pciedev_dev *pdev   = (pciedev_dev*)dev_id;
    struct module_dev *dev     = (module_dev *)(pdev->dev_str);
    
    //printk(KERN_ALERT "PCIEDEV_INTERRUPT:   MSI IRQ 3\n");
    return IRQ_HANDLED;
}

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
static irqreturn_t pciedev_interrupt7(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t pciedev_interrupt7(int irq, void *dev_id)
#endif
{
    uint32_t intreg = 0;
    
    struct pciedev_dev *pdev   = (pciedev_dev*)dev_id;
    struct module_dev *dev     = (module_dev *)(pdev->dev_str);
    
    //printk(KERN_ALERT "PCIEDEV_INTERRUPT:   MSI IRQ 7\n");
    return IRQ_HANDLED;
}

//end MSI 




#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    static int pciedev_probe(struct pci_dev *dev, const struct pci_device_id *id)
#else 
    static int __devinit pciedev_probe(struct pci_dev *dev, const struct pci_device_id *id)
#endif
{  
    int result               = 0;
    int tmp_brd_num   = -1;
    int is_shapi            = 0;
    int i                       = 0;
    int devise_irqs_enabled                       = 0;
    
    printk(KERN_ALERT "PCIEDEV_PROBE CALLED \n");
    result = pciedev_probe_exp(dev, id, &pciedev_fops, pciedev_cdev_m, DEVNAME, &tmp_brd_num);
    printk(KERN_ALERT "PCIEDEV_PROBE_EXP CALLED  FOR BOARD %i\n", tmp_brd_num);
    /*if board has created we will create our structure and pass it to pcedev_dev*/
    if(!result){
        printk(KERN_ALERT "PCIEDEV_PROBE_EXP CREATING CURRENT STRUCTURE FOR BOARD %i\n", tmp_brd_num);
        module_dev_pp = kzalloc(sizeof(module_dev), GFP_KERNEL);
        if(!module_dev_pp){
                return -ENOMEM;
        }
        printk(KERN_ALERT "PCIEDEV_PROBE CALLED; CURRENT STRUCTURE CREATED \n");
        module_dev_p[tmp_brd_num] = module_dev_pp;
        module_dev_pp->brd_num      = tmp_brd_num;
        module_dev_pp->parent_dev  = pciedev_cdev_m->pciedev_dev_m[tmp_brd_num];
        init_waitqueue_head(&module_dev_pp->waitDMA);
        pciedev_set_drvdata(pciedev_cdev_m->pciedev_dev_m[tmp_brd_num], module_dev_p[tmp_brd_num]);
        devise_irqs_enabled                       =  pciedev_cdev_m->pciedev_dev_m[tmp_brd_num]->device_irq_num;
        printk(KERN_ALERT "PCIEDEV_PROBE CALLED; DEVICE IRQs %X \n", devise_irqs_enabled);
        
        is_shapi            = pciedev_cdev_m->pciedev_dev_m[tmp_brd_num]->shapi_brd;
        printk(KERN_ALERT "PCIEDEV_PROBE CALLED; SETTINGIRQs  IS_SHAPI %X \n", is_shapi);
        if(is_shapi){
			for(i =0 ; i < 32; ++i){
				if((devise_irqs_enabled >> i) & 0x1){
					printk(KERN_ALERT "PCIEDEV_PROBE CALLED; SETTINGIRQs  MSI_IRQ_NUM %i \n", i);
					if(i == 0){
						pciedev_setup_interrupt(pciedev_interrupt, pciedev_cdev_m->pciedev_dev_m[tmp_brd_num], DEVNAME, 0); 
					}
					if(i == 2){
						pciedev_setup_interrupt(pciedev_interrupt2, pciedev_cdev_m->pciedev_dev_m[tmp_brd_num], DEVNAME, 2); 
					}
					if(i == 3){
						pciedev_setup_interrupt(pciedev_interrupt3, pciedev_cdev_m->pciedev_dev_m[tmp_brd_num], DEVNAME, 3); 
					}
					if(i == 7){
						pciedev_setup_interrupt(pciedev_interrupt7, pciedev_cdev_m->pciedev_dev_m[tmp_brd_num], DEVNAME, 7); 
					}
				}
			}	
		}else{
			pciedev_setup_interrupt(pciedev_interrupt, pciedev_cdev_m->pciedev_dev_m[tmp_brd_num], DEVNAME, 0); 
	   }
    }
    return result;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    static void pciedev_remove(struct pci_dev *dev)
#else 
   static void __devexit pciedev_remove(struct pci_dev *dev)
#endif
{
     int result               = 0;
     int tmp_slot_num = -1;
     int tmp_brd_num = -1;
     printk(KERN_ALERT "REMOVE CALLED\n");
     tmp_brd_num =pciedev_get_brdnum(dev);
     printk(KERN_ALERT "REMOVE CALLED FOR BOARD %i\n", tmp_brd_num);
     /* clean up any allocated resources and stuff here */
     kfree(module_dev_p[tmp_brd_num]);
     /*now we can call pciedev_remove_exp to clean all standard allocated resources
      will clean all interrupts if it seted 
      */
     result = pciedev_remove_exp(dev,  pciedev_cdev_m, DEVNAME, &tmp_slot_num);
     printk(KERN_ALERT "PCIEDEV_REMOVE_EXP CALLED  FOR SLOT %i\n", tmp_slot_num);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
    static struct pci_driver pci_pciedev_driver = {
    .name       = DEVNAME,
    .id_table   = pciedev_ids,
    .probe      = pciedev_probe,
    .remove    = pciedev_remove,
};
#else 
   static struct pci_driver pci_pciedev_driver = {
    .name       = DEVNAME,
    .id_table   = pciedev_ids,
    .probe      = pciedev_probe,
    .remove     = __devexit_p(pciedev_remove),
};
#endif


static int pciedev_open( struct inode *inode, struct file *filp )
{
    int    result = 0;
    //printk(KERN_ALERT "PCIEDEV_OPEN CALLED\n");
    result = pciedev_open_exp( inode, filp );
    return result;
}

static int pciedev_release(struct inode *inode, struct file *filp)
{
    int result            = 0;
    //printk(KERN_ALERT "PCIEDEV_CLOSE CALLED\n");
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

static int pciedev_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	ssize_t         retval = 0;
	//printk(KERN_ALERT "PCIEDEV_MMAP CALLED\n");
	retval =pciedev_remap_mmap_exp(filp, vma);
	//printk(KERN_ALERT "PCIEDEV_MMAP_EXP CALLED\n");
	return 0;
}

static long  pciedev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    
/*
    if (_IOC_TYPE(cmd) == PCIEDOOCS_IOC){
        if (_IOC_NR(cmd) <= PCIEDOOCS_IOC_MAXNR && _IOC_NR(cmd) >= PCIEDOOCS_IOC_MINNR) {
            result = pciedev_ioctl_exp(filp, &cmd, &arg, pciedev_cdev_m);
        }else{
            result = pciedev_ioctl_dma(filp, &cmd, &arg, pciedev_cdev_m);
        }
    }else{
        return -ENOTTY;
    }
*/
	
	
    
    if (_IOC_TYPE(cmd) == PCIEDOOCS_IOC){
        if (_IOC_NR(cmd) <= PCIEDOOCS_IOC_MAXNR && _IOC_NR(cmd) >= PCIEDOOCS_IOC_MINNR) {
			if (_IOC_NR(cmd) <= PCIEDOOCS_IOC_DMA_MAXNR && _IOC_NR(cmd) >= PCIEDOOCS_IOC_DMA_MINNR) {
				result = pciedev_ioctl_dma(filp, &cmd, &arg, pciedev_cdev_m);
			}else{
				result = pciedev_ioctl_exp(filp, &cmd, &arg, pciedev_cdev_m);
			}
	}else{
		return -ENOTTY;
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
    //result = upciedev_init_module_exp(&pciedev_cdev_m, &pciedev_fops, DEVNAME, p_base_upciedev_dev);
	
    printk(KERN_ALERT "AFTER_INIT:REGISTERING PCI DRIVER\n");
    result = pci_register_driver(&pci_pciedev_driver);
    printk(KERN_ALERT "AFTER_INIT:REGISTERING PCI DRIVER RESUALT %d\n", result);
    return result; /* succeed */
}

module_init(pciedev_init_module);
module_exit(pciedev_cleanup_module);


