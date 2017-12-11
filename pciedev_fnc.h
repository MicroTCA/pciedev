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

#ifndef _PCIEDEV_FNC_H_
#define _PCIEDEV_FNC_H_

#include "pciedev_io.h"
#include "pciedev_ufn.h"

#define DEVNAME "pciedev"	                             /* name of device */
#define PCIEDEV_VENDOR_ID 0x10EE	                    /* XILINX vendor ID */
#define PCIEDEV_DEVICE_ID 0x0088	                    /* PCIEDEV dev board device ID or 0x0088 */
#define PCIEDEV_SUBVENDOR_ID PCI_ANY_ID	 /* ESDADIO vendor ID */
#define PCIEDEV_SUBDEVICE_ID PCI_ANY_ID           /* ESDADIO device ID */

#define DMA_BOARD_ADDRESS     0x4
#define DMA_CPU_ADDRESS          0x8
#define DMA_SIZE_ADDRESS          0xC

struct module_dev {
    int                          brd_num;
//    spinlock_t            irq_lock;
    struct timeval     dma_start_time;
    struct timeval     dma_stop_time;
    int                         waitFlag;
    u32                       dev_dma_size;
    u32                       dma_page_num;
    int                         dma_offset;
    int                         dma_order;
    wait_queue_head_t  waitDMA;
    struct pciedev_dev *parent_dev;
};
typedef struct module_dev module_dev;

long     pciedev_ioctl_dma(struct file *, unsigned int* , unsigned long*, pciedev_cdev * );

#endif /* _PCIEDEV_FNC_H_ */
