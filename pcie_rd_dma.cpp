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

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

#include "pciedev_io.h"

/* useconds from struct timeval */
#define	MIKRS(tv) (((double)(tv).tv_usec ) + ((double)(tv).tv_sec * 1000000.0)) 
#define	MILLS(tv) (((double)(tv).tv_usec/1000 )  + ((double)(tv).tv_sec * 1000.0)) 

int main(int argc, char* argv[])
{

  char                  nod_name[15] = "";
  device_ioctrl_dma     DMA_RW;
  u_int     	        tmp_size;
  u_int     	        read_size;
  u_int	                tmp_offset=0;
  u_int	                word_data=0;
  int                   code = 0;
  int*                  tmp_dma_buf;
  int	                fd;
  int                   numberInts= 8;
  int                   numberReadLoops = 0;
  int                   k = 0;

  if(argc<3){
    printf("\nInput \"pcie_rd_dma [device] [words] [offset] [number int] [mask]\"\nVersion: 1.0\nwords: num of words (int)\noffset (int)\nnumber int: number of integers per word (default 8)\nmask in hexadecimal\n");
    return 0;
  }
  
  DMA_RW.dma_offset  = 0;
  DMA_RW.dma_size    = 0;
  DMA_RW.dma_cmd     = 0;
  DMA_RW.dma_pattern = 0; 

  /* */
  strncpy(nod_name,argv[1],sizeof(nod_name));
  fd = open (nod_name, O_RDWR);
  if (fd < 0) {
    printf ("#CAN'T OPEN DEVICE \n");
    exit (1);
  }

  // Offset
  if (argc > 3) {
    sscanf(argv[3],"%u",&tmp_offset);    
    DMA_RW.dma_offset  = tmp_offset;
  }

  // Number of ints to put together 
  if (argc > 4) {
    sscanf(argv[4],"%d",&numberInts);    
  }

  // Size of DMA 
  sscanf(argv[2],"%u",&tmp_size);
  DMA_RW.dma_size    = sizeof(int)*tmp_size;		  

  // Get DMA data
  tmp_dma_buf     = new int[tmp_size + DMA_DATA_OFFSET];
  memcpy(tmp_dma_buf, &DMA_RW, sizeof (device_ioctrl_dma));

  code = ioctl (fd, PCIEDEV_READ_DMA, tmp_dma_buf);
  
  // Print data
    for(int i = 0; i < tmp_size; i=i+numberInts){
      for(int j=numberInts-1; j>=0; j--)
	printf("%08X",(u_int)(tmp_dma_buf[i+j] & 0xFFFFFFFF));
        printf("\n");
        ;
    }

/*
   k = 0;
   for(int i = 0; i < 40; i++){
      printf("NUM %i OFFSET %X : DATA %X\n", i,k, (u_int)(tmp_dma_buf[i] & 0xFFFFFFFF));
      k += 4;
   }
*/

  if(tmp_dma_buf) delete tmp_dma_buf;
}

  /*
  // Number of 4Mb loops 
  numberReadLoops = tmp_size/MAX_INT_PER_READ;
  
  for (int readLoop=0; readLoop<=numberReadLoops; readLoop++) {
    if (tmp_size > MAX_INT_PER_READ) {
      read_size = MAX_INT_PER_READ;
      tmp_size -= MAX_INT_PER_READ;      
    } else {
      read_size = tmp_size;
    }
    DMA_RW.dma_size = sizeof(int)*read_size;
    tmp_dma_buf       = new int[read_size + DMA_DATA_OFFSET];
    memcpy(tmp_dma_buf, &DMA_RW, sizeof (device_ioctrl_dma));
    code = ioctl (fd, PCIEDEV_READ_DMA, tmp_dma_buf);
    // Join data in words of numberInts (default 8int = 32bytes)
    for(int i = 0; i < read_size; i=i+numberInts){
      for(int j=numberInts-1; j>=0; j--)
	printf("%08X",(u_int)(tmp_dma_buf[i+j] & 0xFFFFFFFF));
      printf("\n");
    }
    if(tmp_dma_buf) delete tmp_dma_buf;
    DMA_RW.dma_offset += read_size;
  }
  */

