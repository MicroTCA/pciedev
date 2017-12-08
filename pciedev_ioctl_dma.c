/**
*Copyright 2016-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
*
*This file is part of PCIEDEV driver.
*
*Foobar is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*Foobar is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
**/


/*
*	Author: Ludwig Petrosyan (Email: ludwig.petrosyan@desy.de)
*/

#include <linux/types.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/swap.h>

#include "pciedev_fnc.h"
#include "pciedev_io.h"
#include "pciedev_ufn.h"

long     pciedev_ioctl_dma(struct file *filp, unsigned int *cmd_p, unsigned long *arg_p, pciedev_cdev * pciedev_cdev_m)
{
    unsigned int    cmd;
    unsigned long arg;
    pid_t                cur_proc = 0;
    int                    minor    = 0;
    int                    d_num    = 0;
    int                    retval   = 0;
    int                    err      = 0;
    struct pci_dev* pdev;
    ulong           value;
    u_int	    tmp_dma_size;
    u_int	    tmp_dma_buf_size;
    u_int	    tmp_dma_trns_size;
    u_int	    tmp_dma_offset;
    u32            tmp_data_32;
    void*           pWriteBuf           = 0;
    void*          dma_reg_address;
    int             tmp_order           = 0;
    unsigned long   length              = 0;
    dma_addr_t      pTmpDmaHandle;
    u32             dma_sys_addr ;
    int               tmp_source_address  = 0;
    u_int           tmp_dma_control_reg = 0;
    u_int           tmp_dma_len_reg     = 0;
    u_int           tmp_dma_src_reg     = 0;
    u_int           tmp_dma_dest_reg    = 0;
    u_int           tmp_dma_cmd         = 0;
    long            timeDMAwait;
    int              tmp_free_pages = 0;
    int              dma_trans_cnt            = 0;
    int              dma_trans_rest           = 0; 
    int              dma_cnt_done            = 0;
    int              dma_usr_byte             = 0;

    int size_time;
    int io_dma_size;
    device_ioctrl_time  time_data;
    device_ioctrl_dma   dma_data;

    module_dev       *module_dev_pp;
    pciedev_dev       *dev  = filp->private_data;
    module_dev_pp = (module_dev*)(dev->dev_str);

    cmd              = *cmd_p;
    arg                = *arg_p;
    size_time      = sizeof (device_ioctrl_time);
    io_dma_size = sizeof(device_ioctrl_dma);
    minor           = dev->dev_minor;
    d_num         = dev->dev_num;	
    cur_proc     = current->group_leader->pid;
    pdev            = (dev->pciedev_pci_dev);
    
    if(!dev->dev_sts){
        printk(KERN_ALERT "PCIEDEV_IOCTRL: NO DEVICE %d\n", dev->dev_num);
        retval = -EFAULT;
        return retval;
    }
        
     /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
     if (_IOC_DIR(cmd) & _IOC_READ)
             err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
     else if (_IOC_DIR(cmd) & _IOC_WRITE)
             err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
     if (err) return -EFAULT;

/*
    if (mutex_lock_interruptible(&dev->dev_mut))
                    return -ERESTARTSYS;
*/
    mutex_lock(&dev->dev_mut);

    switch (cmd) {
        case PCIEDEV_GET_DMA_TIME:
            retval = 0;
            
            module_dev_pp->dma_start_time.tv_sec+= (long)dev->slot_num;
            module_dev_pp->dma_stop_time.tv_sec  += (long)dev->slot_num;
            module_dev_pp->dma_start_time.tv_usec+= (long)dev->brd_num;
            module_dev_pp->dma_stop_time.tv_usec  += (long)dev->brd_num;
            time_data.start_time = module_dev_pp->dma_start_time;
            time_data.stop_time  = module_dev_pp->dma_stop_time;
            if (copy_to_user((device_ioctrl_time*)arg, &time_data, (size_t)size_time)) {
                retval = -EIO;
                mutex_unlock(&dev->dev_mut);
                return retval;
            }
            break;
        case PCIEDEV_READ_DMA:
            retval = 0;
            dma_usr_byte = copy_from_user(&dma_data, (device_ioctrl_dma*)arg, (size_t)io_dma_size);
            //if (copy_from_user(&dma_data, (device_ioctrl_dma*)arg, (size_t)io_dma_size)) {
            if (dma_usr_byte) {
                printk(KERN_ALERT "PCIEDEV_IOCTRL_READ_DMA: COPY FROM USER ERROR  SLOT %d BYTES %i\n", dev->slot_num, dma_usr_byte);
                retval = -EFAULT;
                mutex_unlock(&dev->dev_mut);
                return retval;
            }
             if(!dev->memmory_base[2]){
                printk("PCIEDEV_IOCTL_DMA: NO MEMORY\n");
                retval = -ENOMEM;
                mutex_unlock(&dev->dev_mut);
                return retval;
            }
            dma_reg_address = dev->memmory_base[2];

            tmp_dma_control_reg = (dma_data.dma_reserved1 >> 16) & 0xFFFF;
            tmp_dma_len_reg     = dma_data.dma_reserved1 & 0xFFFF;
            tmp_dma_src_reg     = (dma_data.dma_reserved2 >> 16) & 0xFFFF;
            tmp_dma_dest_reg    = dma_data.dma_reserved2 & 0xFFFF;
            tmp_dma_cmd         = dma_data.dma_cmd;
            tmp_dma_size        = dma_data.dma_size;
            tmp_dma_offset      = dma_data.dma_offset;
            length   = tmp_dma_size;


/*
           printk (KERN_ALERT "PCIEDEV_READ_DMA: tmp_dma_cmd %X, tmp_dma_size %X\n",
                   tmp_dma_cmd, tmp_dma_size);

*/
            
            module_dev_pp->dev_dma_size     = tmp_dma_size;
             if(tmp_dma_size <= 0){
                 mutex_unlock(&dev->dev_mut);
                 printk(KERN_ALERT "PCIEDEV_IOCTRL_READ_DMA: DMA SIZE <= 0 SLOT %d\n", dev->slot_num);
                 return EFAULT;
            }
            
/*
            if(tmp_dma_size > dev->rw_off2){
                  printk (KERN_ALERT "PCIEDEV_READ_DMA:DMA_SIZE %i BIGGER THAN MEMORY SIZE %u\n", tmp_dma_size, dev->rw_off2);
                 mutex_unlock(&dev->dev_mut);
                 return EFAULT;
            }
*/
            
            tmp_free_pages = nr_free_pages();
            tmp_free_pages = tmp_free_pages << (PAGE_SHIFT-10);
            tmp_free_pages = tmp_free_pages/2;
            
            tmp_dma_buf_size    = tmp_dma_size;
            if((tmp_dma_buf_size%PCIEDEV_DMA_SYZE)){
                tmp_dma_buf_size    = tmp_dma_size + (PCIEDEV_DMA_SYZE - tmp_dma_size%PCIEDEV_DMA_SYZE);
            }
            tmp_dma_trns_size = tmp_dma_buf_size;
            dma_trans_cnt   = 0;
            dma_trans_rest  = tmp_dma_size; 
            
            

/*
            printk (KERN_ALERT "PCIEDEV_READ_DMA: PAGES AVAILABLE %i MEM REQUSTED %i MEM WILL ALOCCATE %i\n",
                    tmp_free_pages, tmp_dma_size, tmp_dma_trns_size);

*/
             if((tmp_free_pages%32)){
                tmp_free_pages    = 32 * (tmp_free_pages/32);
            }

            if(tmp_dma_size > tmp_free_pages){
                dma_trans_cnt          = tmp_dma_size/tmp_free_pages;
                dma_trans_rest         = tmp_dma_size - dma_trans_cnt*tmp_free_pages;
                tmp_dma_size           = tmp_free_pages;
                tmp_dma_trns_size   = tmp_free_pages;
/*
                printk (KERN_ALERT "PCIEDEV_READ_DMA: REQUESTED MORE THAN AVAILABLE dma_trans_cnt %i,  dma_trans_rest %i tmp_dma_size %i\n",
                    dma_trans_cnt, dma_trans_rest, tmp_dma_size);
*/

            }
            dma_cnt_done            = 0;
            
/*
            printk (KERN_ALERT "PCIEDEV_READ_DMA: PAGES AVAILABLE %i MEM REQUSTED %i MEM WILL ALOCCATE %i\n",
                    tmp_free_pages, tmp_dma_size, tmp_dma_trns_size);
            printk (KERN_ALERT "PCIEDEV_READ_DMA: DMA_CNTE %i DMA_REST %i DMA_SIZE %i\n",
                    dma_trans_cnt, dma_trans_rest, tmp_dma_size);
*/
            
            //value    = HZ/1; /* value is given in jiffies*/
	   value  = 10000*HZ/150000; /* value is given in jiffies*/
            tmp_order = get_order(tmp_dma_trns_size);
            module_dev_pp->dma_order = tmp_order;
            pWriteBuf = (void *)__get_free_pages(GFP_KERNEL | __GFP_DMA, tmp_order);
            
            if (!pWriteBuf){
                printk (KERN_ALERT "PCIEDEV_READ_DMA: NO MEMORY FOR SIZE,  %X SLOT %d\n",tmp_dma_size, dev->slot_num);
                mutex_unlock(&dev->dev_mut);
                return EFAULT;
            }
            
            for(dma_cnt_done = 0; dma_cnt_done <= dma_trans_cnt; dma_cnt_done++){
                pTmpDmaHandle      = pci_map_single(pdev, pWriteBuf, tmp_dma_trns_size, PCI_DMA_FROMDEVICE);
               
                /* MAKE DMA TRANSFER*/
                tmp_source_address = tmp_dma_offset + dma_cnt_done*tmp_dma_size;
                dma_sys_addr       = (u32)(pTmpDmaHandle & 0xFFFFFFFF);
                iowrite32(tmp_source_address, ((void*)(dma_reg_address + DMA_BOARD_ADDRESS)));
                tmp_data_32         = dma_sys_addr;
                iowrite32(tmp_data_32, ((void*)(dma_reg_address + DMA_CPU_ADDRESS)));
                smp_wmb();
                udelay(5);
                tmp_data_32       = ioread32(dma_reg_address + 0x0); // be safe all writes are done
                smp_rmb();
/*
                tmp_data_32         = tmp_dma_size;
                 if(dma_cnt_done == dma_trans_cnt){
                    tmp_data_32    = dma_trans_rest;
                }
*/
                tmp_data_32         = tmp_dma_size;
                 if(dma_cnt_done == dma_trans_cnt){
                 //tmp_dma_size    = dma_trans_rest;
		         tmp_data_32         = dma_trans_rest;
                }
                
                
 /*               
                printk (KERN_ALERT "PCIEDEV_READ_DMA:SLOT NUM %i SYS_ADDRESS %X BOARD+ADDRESS %X:%i \n", 
                            dev->slot_num, dma_sys_addr, tmp_source_address, tmp_source_address);
                
*/                
                do_gettimeofday(&(module_dev_pp->dma_start_time));
                module_dev_pp->waitFlag = 0;
                iowrite32(tmp_data_32, ((void*)(dma_reg_address + DMA_SIZE_ADDRESS)));
                //timeDMAwait = wait_event_interruptible_timeout( module_dev_pp->waitDMA, module_dev_pp->waitFlag != 0, value );
	            timeDMAwait = wait_event_timeout( module_dev_pp->waitDMA, module_dev_pp->waitFlag != 0, value );
                do_gettimeofday(&(module_dev_pp->dma_stop_time));
                 if(!module_dev_pp->waitFlag){
                    printk (KERN_ALERT "PCIEDEV_READ_DMA:SLOT NUM %i NO INTERRUPT \n", dev->slot_num);
                    module_dev_pp->waitFlag = 1;
                    pci_unmap_single(pdev, pTmpDmaHandle, tmp_dma_trns_size, PCI_DMA_FROMDEVICE);
                    free_pages((ulong)pWriteBuf, (ulong)module_dev_pp->dma_order);
                    mutex_unlock(&dev->dev_mut);
                    return EFAULT;
                }
                pci_unmap_single(pdev, pTmpDmaHandle, tmp_dma_trns_size, PCI_DMA_FROMDEVICE);
    
/*                
                printk (KERN_ALERT "PCIEDEV_READ_DMA:SLOT NUM %i COPY TO %X:%i COPY_FROM %X COPY _SIZE %i \n", 
                            dev->slot_num, dma_cnt_done*tmp_dma_size, dma_cnt_done*tmp_dma_size, pWriteBuf, tmp_data_32);
*/                
                
                dma_usr_byte = copy_to_user ( ((void *)arg + dma_cnt_done*tmp_dma_size), pWriteBuf, tmp_data_32);
                 //if (copy_to_user ( ((void *)arg + dma_cnt_done*tmp_dma_size), pWriteBuf, tmp_dma_size) ) {
                 if (dma_usr_byte) {
                    printk(KERN_ALERT "PCIEDEV_IOCTRL_READ_DMA: COPY TO USER ERROR SLOT %d COUNT %d SIZE %d OFFSET IN BUF %d BYTES %i\n", 
                            dev->slot_num, dma_cnt_done, tmp_data_32, dma_cnt_done*tmp_dma_size, dma_usr_byte);
                    mutex_unlock(&dev->dev_mut);
                    retval = -EFAULT;
                }
            }
            free_pages((ulong)pWriteBuf, (ulong)module_dev_pp->dma_order);
            break;
        default:
            printk(KERN_ALERT "PCIEDEV_IOCTRL: NO IOCTRL %d\n", dev->slot_num);
            mutex_unlock(&dev->dev_mut);
            return -ENOTTY;
            break;
    }
    mutex_unlock(&dev->dev_mut);
    return retval;
}
