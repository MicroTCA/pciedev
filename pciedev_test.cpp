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


int	         fd;
struct timeval   start_time;
struct timeval   end_time;

int main(int argc, char* argv[])
{
	int	 ch_in        = 0;
	char nod_name[15] = "";
	device_rw	               l_Read;
	device_ioctrl_data     io_RW;
	device_ioctrl_dma     DMA_RW;
	device_ioctrl_time     DMA_TIME;
	device_ioc_rw            ioc_ACCESS;
	device_vector_rw       ioc_VREAD;
	u_int			  tmp_mode;
	u_int	                    tmp_offset;
	u_int      	          tmp_data;
	u_int      	          tmp_barx;
	float                     tmp_fdata;
	int                   len = 0;
	int                   k = 0;
	int                   itemsize = 0;
	int*              tmp_dma_buf;
	u_int*              tmp_rw_buf;
	char*              ctmp_rw_buf;
	u_int	          tmp_size;
	int                code = 0;
	double          time_tmp = 0;
	double          time_dlt;
	int      	  tmp_print = 0;
	int      	  tmp_print_start = 0;
	int      	  tmp_print_stop  = 0;
	int            loop_delay = 0;
	int           mem_incr = 0;
	float        drv_version;
	u_int        tmp_dataPtr;	// In the case of more than one register access
	u_int        tmp_maskPtr;
	u_int*                        tmp_vrw_buf[6];
	device_ioc_rw            vrw_ioc_ACCESS[6];
	int                             vrw_count;

	itemsize = sizeof(device_rw);
	printf("ITEMSIZE %i \n",itemsize);

	if(argc ==1){
		printf("Input \"prog /dev/damc0\" \n");
		return 0;
	}
	printf("START OPEN FILE \n");
	strncpy(nod_name,argv[1],sizeof(nod_name));
	printf("FILE %s \n",nod_name);
	fd = open (nod_name, O_RDWR);
	printf("OPEN FILE ID %i \n", fd);

	if (fd < 0) {
		printf ("#CAN'T OPEN FILE \n");
		exit (1);
	}
	
	ioctl(fd, PCIEDEV_DRIVER_VERSION, &io_RW);
	drv_version = (float)((float)io_RW.offset/10.0);
	drv_version += (float)io_RW.data;
	printf ("DRIVER VERSION IS %f\n", drv_version);

	
	tmp_rw_buf   = 0;
	ctmp_rw_buf  = 0;
	while (ch_in != 11){
		printf("\n READ (1) or WRITE (0) ?-");
		printf("\n GET DRIVER VERSION (2) or GET FIRMWARE VERSION (3) ?-");
		printf("\n GET SLOT NUM (4) or GET_STATUS (5) ?-");
		printf("\n SET_BITS (6) or SWAP_BITS(7)  VECTOR_READ (8)?-");
		
		printf("For testing!!! Do not use it Read Open/Close (9)");
		
		printf("\n SET_SCRATC_REG (12) or GET_SRATCH_REG(13) or READ_SCRATCH_REG (14) ?-");
		
		printf("\n GET SWAP (15) or  SET_SWAP (16)  ?-");
		printf("\n GET REGISTERSIZE (17) or SET_REGISTERSIZE (18) ?-");
		
		printf("\n PREAD (26) or PWRITE (27) LLSEEK (28) ?-");
		printf("\n GET_DMA_TIME (29) ?-");
		printf("\n CTRL_DMA READ (30) CTRL_DMA WRITE (31) CTRL_DMA LOOP_READ (32)?-");
		printf("\n END (11) ?-");
		scanf("%d",&ch_in);
		fflush(stdin);
		l_Read.offset_rw   = 0;
		l_Read.data_rw     = 0;
		l_Read.mode_rw     = 0;
		l_Read.barx_rw     = 0;
		l_Read.size_rw     = 0;
		l_Read.rsrvd_rw    = 0;
		switch (ch_in){
			case 0 :
				printf ("\n INPUT  BARx (0,1,2,3...)  -");
				scanf ("%x",&tmp_barx);
				fflush(stdin);

				printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
				scanf ("%x",&tmp_mode);
				fflush(stdin);

				printf ("\n INPUT  ADDRESS (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);

				printf ("\n INPUT DATA (IN HEX)  -");
				scanf ("%x",&tmp_data);
				fflush(stdin);

				l_Read.data_rw   = tmp_data;
				l_Read.offset_rw = tmp_offset;
				l_Read.mode_rw   = tmp_mode;
				l_Read.barx_rw   = tmp_barx;
				l_Read.size_rw   = 0;
				l_Read.rsrvd_rw  = 0;

				printf ("MODE - %X , OFFSET - %X, DATA - %X\n", l_Read.mode_rw, l_Read.offset_rw, 
						                                                                                         l_Read.data_rw);

				len = write (fd, &l_Read, sizeof(device_rw));
				if (len < 0 ){
					printf ("#CAN'T READ FILE return %i\n", len);
				}

				break;
			case 1 :
				tmp_size = 0;
				printf ("\n INPUT  BARx (0,1,2,3)  -");
				scanf ("%u",&tmp_barx);
				fflush(stdin);
				printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
				scanf ("%u",&tmp_mode);
				fflush(stdin);
				printf ("\n INPUT OFFSET (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				printf ("\n INPUT COUNT)  -");
				scanf ("%x",&tmp_size);
				fflush(stdin);
				if(tmp_size < 2) tmp_size = 1;
				l_Read.data_rw     = 0;
				l_Read.offset_rw   = tmp_offset;
				l_Read.mode_rw   = tmp_mode;
				l_Read.barx_rw    = tmp_barx;
				l_Read.size_rw     = tmp_size;
				l_Read.rsrvd_rw   = 0;
				tmp_rw_buf     = new u_int[tmp_size];
				printf ("MODE - %u , OFFSET - %X, DATA - %u SIZE %u\n", 
					l_Read.mode_rw, l_Read.offset_rw,  l_Read.data_rw, l_Read.size_rw);     
				gettimeofday(&start_time, 0);
				for(k = 0; k < tmp_size; ++k){
					l_Read.offset_rw   = tmp_offset = k*4;
					l_Read.data_rw     = 0;
					l_Read.mode_rw   = tmp_mode;
					l_Read.barx_rw    = tmp_barx;
					l_Read.size_rw     = 1;
					l_Read.rsrvd_rw   = 0;
					len = read (fd, &l_Read, 1);
					*((u_int*)tmp_rw_buf + k*1) = l_Read.data_rw;
				}
				gettimeofday(&end_time, 0);
				printf ("===========READED  CODE %i\n", len);
				time_tmp    =  MIKRS(end_time) - MIKRS(start_time);
				time_dlt       =  MILLS(end_time) - MILLS(start_time);
				printf("STOP READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
				printf("STOP READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
				if (len < 0){
				printf ("#CAN'T READ FILE return %i\n", len);
				}
				tmp_print = 0;
				printf ("PRINT (0 NO, 1 YES)  -\n");
				scanf ("%d",&tmp_print);
				fflush(stdin);
				while (tmp_print){
					printf ("START POS  -\n");
					scanf ("%d",&tmp_print_start);
					fflush(stdin);
					printf ("STOP POS  -\n");
					scanf ("%d",&tmp_print_stop);
					fflush(stdin);
					k = tmp_print_start*4;
					for(int i = tmp_print_start; i < tmp_print_stop; i++){
						printf ("READED : MODE - %X , OFFSET - %X, DATA - %X\n", 
					                              l_Read.mode_rw,k, *((u_int*)tmp_rw_buf + i));
						k += 4;
					}
					printf ("PRINT (0 NO, 1 YES)  -\n");
					scanf ("%d",&tmp_print);
					fflush(stdin);
				}
				if(tmp_rw_buf) delete tmp_rw_buf;
				tmp_rw_buf   = 0;
				break;
			case 2 :
				tmp_mode = 0;
				printf ("\n UPCIEDEV 0, PCIEDEV 1 ? -");
				scanf ("%u",&tmp_mode);
				fflush(stdin);
				if(tmp_mode){
					ioctl(fd, PCIEDEV_DRIVER_VERSION, &io_RW);
				}else{
					ioctl(fd, PCIEDEV_UDRIVER_VERSION, &io_RW);
				}
				tmp_fdata = (float)((float)io_RW.offset/10.0);
				tmp_fdata += (float)io_RW.data;
				printf ("DRIVER VERSION IS %f\n", tmp_fdata);
				break;
			case 3 :
				ioctl(fd, PCIEDEV_FIRMWARE_VERSION, &io_RW);
				printf ("FIRMWARE VERSION IS - %X\n", io_RW.data);
				break;
			case 4 :
				ioctl(fd, PCIEDEV_PHYSICAL_SLOT, &io_RW);
				printf ("SLOT NUM IS - %X\n", io_RW.data);
				break;
			case 5 :
				ioctl(fd, PCIEDEV_GET_STATUS, &io_RW);
				printf ("SLOT NUM IS - %X\n", io_RW.data);
				break;
			
			case 6 :
				ioc_ACCESS.register_size = 0;	/* (RW_D8, RW_D16, RW_D32)      */
				ioc_ACCESS.rw_access_mode = 0; /* mode of rw (MTCA_SIMLE_READ,...)      */
				ioc_ACCESS.barx_rw = 0;	        /* BARx (0, 1, 2, 3, 4, 5)                 */
				ioc_ACCESS.offset_rw = 0;	        /* offset in address                       */
				ioc_ACCESS.count_rw = 0;	        /* number of register to read or write     */
				ioc_ACCESS.dataPtr =(pointer_type)&tmp_dataPtr;	       // In the case of more than one register access
				ioc_ACCESS.maskPtr = (pointer_type)&tmp_maskPtr;	       // mask for bitwise write operations
				tmp_mode = 0;
				printf ("\n BAR NUM ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.barx_rw = tmp_mode;
				printf ("\n REG_SIZE (0-8bit, 1-16bit, 2-32bit) ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.register_size = tmp_mode;
				
				printf ("\n OFFSET (HEX) ? -");
				scanf ("%X",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.offset_rw = tmp_mode;
				printf ("\n COUNT ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.count_rw = tmp_mode;
				printf ("\n DATA(HEX) ? -");
				scanf ("%X",&tmp_mode);
				fflush(stdin);
				tmp_dataPtr = tmp_mode;
				printf ("\n MASK(HEX) ? -");
				scanf ("%X",&tmp_mode);
				fflush(stdin);
				tmp_maskPtr = tmp_mode;
				
				len = ioctl(fd, PCIEDEV_SET_BITS, &ioc_ACCESS);
				if (len < 0 ){
					printf ("#CAN'T READ FILE return %i\n", len);
				}
				break;
        
			case 7 :
				ioc_ACCESS.register_size = 0;	/* (RW_D8, RW_D16, RW_D32)      */
				ioc_ACCESS.rw_access_mode = 0; /* mode of rw (MTCA_SIMLE_READ,...)      */
				ioc_ACCESS.barx_rw = 0;	        /* BARx (0, 1, 2, 3, 4, 5)                 */
				ioc_ACCESS.offset_rw = 0;	        /* offset in address                       */
				ioc_ACCESS.count_rw = 0;	        /* number of register to read or write     */
				ioc_ACCESS.dataPtr =(pointer_type)&tmp_dataPtr;	       // In the case of more than one register access
				ioc_ACCESS.maskPtr = (pointer_type)&tmp_maskPtr;	       // mask for bitwise write operations
				tmp_mode = 0;
				printf ("\n BAR NUM ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.barx_rw = tmp_mode;
				printf ("\n REG_SIZE (0-8bit, 1-16bit, 2-32bit) ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.register_size = tmp_mode;
				
				printf ("\n OFFSET (HEX) ? -");
				scanf ("%X",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.offset_rw = tmp_mode;
				printf ("\n COUNT ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioc_ACCESS.count_rw = tmp_mode;
//				printf ("\n DATA(HEX) ? -");
//				scanf ("%X",&tmp_mode);
//				fflush(stdin);
//				tmp_dataPtr = tmp_mode;
				printf ("\n MASK(HEX) ? -");
				scanf ("%X",&tmp_mode);
				fflush(stdin);
				tmp_maskPtr = tmp_mode;
				tmp_dataPtr  = tmp_mode;
				
				len = ioctl(fd, PCIEDEV_SWAP_BITS, &ioc_ACCESS);
				if (len < 0 ){
					printf ("#CAN'T READ FILE return %i\n", len);
				}
				break;
				
			case 8:
				ioc_VREAD.number_of_rw      = 0;
				ioc_VREAD.device_ioc_rw_ptr = 0;
				vrw_count = 0;
				printf ("\n NUMBER OF READS (max 6) ? -");
				scanf ("%i",&vrw_count);
				fflush(stdin);
				ioc_VREAD.number_of_rw      = vrw_count;
				ctmp_rw_buf  = 0;
				ctmp_rw_buf                 = new char[sizeof(device_ioc_rw)*vrw_count];
				ioc_VREAD.device_ioc_rw_ptr = (pointer_type)ctmp_rw_buf;
				//tmp_vrw_buf[6]     = new u_int[tmp_size];
				for(int i = 0; i< vrw_count; ++i){
					vrw_ioc_ACCESS[i].rw_access_mode = 0; /* mode of rw (MTCA_SIMLE_READ,...)      */
					tmp_mode = 0;
					printf ("\n BAR NUM ? -");
					scanf ("%i",&tmp_mode);
					fflush(stdin);
					vrw_ioc_ACCESS[i].barx_rw = tmp_mode;
					printf ("\n REG_SIZE (0-8bit, 1-16bit, 2-32bit) ? -");
					scanf ("%i",&tmp_mode);
					fflush(stdin);
					vrw_ioc_ACCESS[i].register_size = tmp_mode;
					printf ("\n OFFSET (HEX) ? -");
					scanf ("%X",&tmp_mode);
					fflush(stdin);
					vrw_ioc_ACCESS[i].offset_rw = tmp_mode;
					printf ("\n COUNT ? -");
					scanf ("%i",&tmp_mode);
					fflush(stdin);
					vrw_ioc_ACCESS[i].count_rw = tmp_mode;
					tmp_vrw_buf[i] = 0;
					tmp_vrw_buf[i] = new u_int[tmp_mode];
					vrw_ioc_ACCESS[i].dataPtr =(pointer_type)tmp_vrw_buf[i];
					memcpy((ctmp_rw_buf + i* sizeof (device_ioc_rw)), &vrw_ioc_ACCESS[i], sizeof (device_ioc_rw));
				}
				for(int i = 0; i< vrw_count; ++i){
					printf ("#####READ NUM %i\n", i);
					printf ("REG_SIZE %i BAR %i OFFSET %X COUNT %i\n", 
						*(int*)((char*)ctmp_rw_buf + i* sizeof (device_ioc_rw)),
						*(int*)((char*)ctmp_rw_buf + i* sizeof (device_ioc_rw) +4),	
						*(int*)((char*)ctmp_rw_buf + i* sizeof (device_ioc_rw) +8),	
						*(int*)((char*)ctmp_rw_buf + i* sizeof (device_ioc_rw) +12)
							);
					;
				}
				gettimeofday(&start_time, 0);
				len = ioctl(fd, PCIEDEV_VECTOR_RW, &ioc_VREAD);
				if (len < 0 ){
					printf ("#CAN'T READ FILE return %i\n", len);
				}
				gettimeofday(&end_time, 0);
				printf ("===========READED  CODE %i\n", len);
				time_tmp    =  MIKRS(end_time) - MIKRS(start_time);
				time_dlt       =  MILLS(end_time) - MILLS(start_time);
				printf("STOP READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
				printf("STOP READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
				
				tmp_print = 0;
				printf ("PRINT (0 NO, 1 YES)  -\n");
				scanf ("%d",&tmp_print);
				fflush(stdin);
				while (tmp_print){
					printf ("START POS  -\n");
					scanf ("%d",&tmp_print_start);
					fflush(stdin);
					printf ("STOP POS  -\n");
					scanf ("%d",&tmp_print_stop);
					fflush(stdin);
					k = tmp_print_start*4;
					for(int i = tmp_print_start; i < tmp_print_stop; i++){
						printf ("VREADED :NUM %i, BAR %i,  MODE - %X , OFFSET - %X, DATA - %X\n", 
			                            i, vrw_ioc_ACCESS[i].barx_rw, vrw_ioc_ACCESS[i].register_size ,k, *((u_int*)vrw_ioc_ACCESS[i].dataPtr + i));
						k += 4;
					}
					printf ("PRINT (0 NO, 1 YES)  -\n");
					scanf ("%d",&tmp_print);
					fflush(stdin);
				}
				
				for(int i = 0; i< vrw_count; ++i){
					if(tmp_vrw_buf[i]) delete tmp_vrw_buf[i];
					tmp_vrw_buf[i] = 0;
				}
				if(ctmp_rw_buf) delete ctmp_rw_buf;
				ctmp_rw_buf  = 0;
				break;
        
           case 9 :
                printf ("\n INPUT  BARx (0,1,2,3...)  -");
                scanf ("%x",&tmp_barx);
                fflush(stdin);

                printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
                scanf ("%x",&tmp_mode);
                fflush(stdin);

                printf ("\n INPUT  ADDRESS (IN HEX)  -");
                scanf ("%x",&tmp_offset);
                fflush(stdin);
                
                printf ("\n INPUT SIZE (IN byte)  -");
                scanf ("%u",&tmp_size);

                printf ("BAR - %X MODE - %X , OFFSET - %X, SIZE - %i\n", tmp_barx, tmp_mode, tmp_offset, tmp_size);
                len = pread(fd, &tmp_data, tmp_size, tmp_offset);

                if (len != tmp_size ){
                        printf ("#CAN'T PREAD FILE return %i\n", len);
                }
                break;
	    case 10 :
                printf ("\n INPUT  BARx (0,1,2,3...)  -");
                scanf ("%x",&tmp_barx);
                fflush(stdin);

                printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
                scanf ("%x",&tmp_mode);
                fflush(stdin);

                printf ("\n INPUT  ADDRESS (IN HEX)  -");
                scanf ("%x",&tmp_offset);
                fflush(stdin);
                
                printf ("\n INPUT SIZE (IN byte)  -");
                scanf ("%u",&tmp_size);
                
                printf ("\n INPUT DATA (IN HEX)  -");
                scanf ("%x",&tmp_data);
                fflush(stdin);

                printf ("BAR - %X MODE - %X , OFFSET - %X, SIZE - %i\n", tmp_barx, tmp_mode, tmp_offset, tmp_size);
                len = pwrite(fd, &tmp_data, tmp_size, tmp_offset);

                if (len != tmp_size ){
                        printf ("#CAN'T PWRITE FILE return %i\n", len);
                }
		break;
        
           
        case 12 :
                  io_RW.cmd = 0;
                  printf ("\n INPUT  SCRATCH BAR  -");
                  scanf ("%i",&tmp_offset);
                  fflush(stdin);
                  io_RW.data = tmp_offset;
                  printf ("\n INPUT  SCRATCH OFFSET  -");
                  scanf ("%i",&tmp_offset);
                  fflush(stdin);
                  io_RW.offset = tmp_offset;
                  ioctl(fd, PCIEDEV_SCRATCH_REG, &io_RW);
		break;
        case 13 :
                  io_RW.cmd   = 1;
                  io_RW.data   = 0;
                  io_RW.offset = 0;
                  ioctl(fd, PCIEDEV_SCRATCH_REG, &io_RW);
                  printf ("SCRATCH BAR = %i, SCRATCH OFFSET = %i\n", io_RW.data, io_RW.offset);
		break;
        case 14 :
		io_RW.cmd   = 2;
                  io_RW.data   = 0;
                  io_RW.offset = 0;
                  ioctl(fd, PCIEDEV_SCRATCH_REG, &io_RW);
                  printf ("SCRATCH DATA = %X\n", io_RW.data);
		break;
		
		
			case 15 :
				len =ioctl(fd, PCIEDEV_GET_SWAP, &io_RW);
				printf ("SWAP IS - %i\n", len);
				break;
			case 16 :
				tmp_mode = 0;
				printf ("\n SET SWAP (0/1) ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				ioctl(fd, PCIEDEV_SET_SWAP, &tmp_mode);
				break;
			case 17 :
				tmp_mode = 0;
				len =ioctl(fd, PCIEDEV_GET_REGISTER_SIZE, &tmp_mode);
				printf ("REGISTER SIZE IS - %i\n", len);
				break;
			case 18 :
				tmp_mode = 0;
				printf ("\n SET REGISTERSIZE (0,1,2) ? -");
				scanf ("%i",&tmp_mode);
				fflush(stdin);
				len =ioctl(fd, PCIEDEV_SET_REGISTER_SIZE, &tmp_mode);
				printf ("REGISTER SIZE IS - %i\n", len);
				break;
			case 28 :
				printf ("\n INPUT  ADDRESS (IN HEX)  -");
				scanf ("%x",&tmp_offset);
				fflush(stdin);
				len = lseek(fd, tmp_offset, 0);
				printf ("LSEEK RETURN %i\n", len);
				break;
				
			case 29:
				len = ioctl (fd, PCIEDEV_GET_DMA_TIME, &DMA_TIME);
				if (len) {
					printf ("######ERROR GET TIME %d\n", len);
				}
				printf ("===========DRIVER TIME \n");
				printf("STOP DRIVER TIME START %li:%li STOP %li:%li\n",
						DMA_TIME.start_time.tv_sec, DMA_TIME.start_time.tv_usec, 
						DMA_TIME.stop_time.tv_sec, DMA_TIME.stop_time.tv_usec);
				break;
			
            case 30 :
                DMA_RW.dma_offset    = 0;
                DMA_RW.dma_size       = 0;
                DMA_RW.dma_cmd      = 0;
                DMA_RW.dma_pattern = 0; 
                printf ("\n INPUT  DMA_SIZE (num of sumples (int))  -");
                scanf ("%u",&tmp_size);
                fflush(stdin);
                printf ("\n ENTERED SIZE %u:%u (int)  -",tmp_size, tmp_size*4);
                DMA_RW.dma_size    = sizeof(int)*tmp_size;
                printf ("\n INPUT OFFSET (int)  -");
                scanf ("%u",&tmp_offset);
                fflush(stdin);
                DMA_RW.dma_offset = tmp_offset;
                printf ("DMA_OFFSET - %X, DMA_SIZE - %u\n", DMA_RW.dma_offset, DMA_RW.dma_size);                
                
                tmp_dma_buf     = new int[tmp_size + DMA_DATA_OFFSET];
                if(tmp_dma_buf){
                    memcpy(tmp_dma_buf, &DMA_RW, sizeof (device_ioctrl_dma));
                    for(int i = 0; i < 4; i++){
                            printf("NUM %i BUF_DATA %X\n", i, (u_int)(tmp_dma_buf[i] & 0xFFFFFFFF));
                    }
                
                }else{
                    printf ("COULD NOT GET BUF MEM\n"); 
                }
               
                gettimeofday(&start_time, 0);
                code = ioctl (fd, PCIEDEV_READ_DMA, tmp_dma_buf);
                gettimeofday(&end_time, 0);
                printf ("===========READED  CODE %i\n", code);
                time_tmp    =  MIKRS(end_time) - MIKRS(start_time);
                time_dlt       =  MILLS(end_time) - MILLS(start_time);
                printf("STOP READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
                printf("STOP READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
                code = ioctl (fd, PCIEDEV_GET_DMA_TIME, &DMA_TIME);
                if (code) {
                    printf ("######ERROR GET TIME %d\n", code);
                }
                printf ("===========DRIVER TIME \n");
                time_tmp = MIKRS(DMA_TIME.stop_time) - MIKRS(DMA_TIME.start_time);
                time_dlt    = MILLS(DMA_TIME.stop_time) - MILLS(DMA_TIME.start_time);
                printf("STOP DRIVER TIME START %li:%li STOP %li:%li\n",
                                                            DMA_TIME.start_time.tv_sec, DMA_TIME.start_time.tv_usec, 
                                                            DMA_TIME.stop_time.tv_sec, DMA_TIME.stop_time.tv_usec);
                printf("STOP DRIVER READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
                printf("STOP DRIVER READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
                printf ("PRINT (0 NO, 1 YES)  -\n");
                scanf ("%d",&tmp_print);
                fflush(stdin);
                while (tmp_print){
                    printf ("START POS  -\n");
                    scanf ("%d",&tmp_print_start);
                    fflush(stdin);
                    printf ("STOP POS  -\n");
                    scanf ("%d",&tmp_print_stop);
                    fflush(stdin);
                    k = tmp_print_start*4;
                    for(int i = tmp_print_start; i < tmp_print_stop; i++){
                            printf("NUM %i OFFSET %X : DATA %X\n", i,k, (u_int)(tmp_dma_buf[i] & 0xFFFFFFFF));
                            k += 4;
                    }
                    printf ("PRINT (0 NO, 1 YES)  -\n");
		    scanf ("%d",&tmp_print);
		    fflush(stdin);
                }
                if(tmp_dma_buf) delete tmp_dma_buf;
                break;
            case 32 :
                DMA_RW.dma_offset  = 0;
                DMA_RW.dma_size    = 0;
                DMA_RW.dma_cmd     = 0;
                DMA_RW.dma_pattern = 0; 
                printf ("\n INPUT  DMA_SIZE (num of sumples (int))  -");
                scanf ("%d",&tmp_size);
                fflush(stdin);
                DMA_RW.dma_size    = sizeof(int)*tmp_size;
                printf ("\n INPUT OFFSET (int)  -");
                scanf ("%d",&tmp_offset);
                fflush(stdin);
                DMA_RW.dma_offset = tmp_offset;
                printf ("\n INPUT LOOP DELAY (usec)  -");
                scanf ("%d",&loop_delay);
                fflush(stdin);
                
                printf ("DMA_OFFSET - %X, DMA_SIZE - %X\n", DMA_RW.dma_offset, DMA_RW.dma_size);
                printf ("MAX_MEM- %X, DMA_MEM - %X:%X\n", 536870912,  (DMA_RW.dma_offset + DMA_RW.dma_size),
                                                                                              (DMA_RW.dma_offset + DMA_RW.dma_size*4));
                
                
                tmp_dma_buf     = new int[tmp_size + DMA_DATA_OFFSET];
                
                
                while(1){
                    DMA_RW.dma_offset  = tmp_offset;
                    DMA_RW.dma_size    = sizeof(int)*tmp_size;
                    DMA_RW.dma_cmd     = 0;
                    DMA_RW.dma_pattern = 0; 
                    memcpy(tmp_dma_buf, &DMA_RW, sizeof (device_ioctrl_dma));

                    gettimeofday(&start_time, 0);
                    code = ioctl (fd, PCIEDEV_READ_DMA, tmp_dma_buf);
                    gettimeofday(&end_time, 0);
                    printf ("===========READED  CODE %i\n", code);
                    time_tmp    =  MIKRS(end_time) - MIKRS(start_time);
                    time_dlt       =  MILLS(end_time) - MILLS(start_time);
                    printf("STOP READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
                    printf("STOP READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
//                    code = ioctl (fd, PCIEDEV_GET_DMA_TIME, &DMA_TIME);
//                    if (code) {
//                        printf ("######ERROR GET TIME %d\n", code);
//                    }
//                    printf ("===========DRIVER TIME \n");
//                    time_tmp = MIKRS(DMA_TIME.stop_time) - MIKRS(DMA_TIME.start_time);
//                    time_dlt    = MILLS(DMA_TIME.stop_time) - MILLS(DMA_TIME.start_time);
//                    printf("STOP DRIVER TIME START %li:%li STOP %li:%li\n",
//                                                                DMA_TIME.start_time.tv_sec, DMA_TIME.start_time.tv_usec, 
//                                                                DMA_TIME.stop_time.tv_sec, DMA_TIME.stop_time.tv_usec);
//                    printf("STOP DRIVER READING TIME %fms : %fmks  SIZE %lu\n", time_dlt, time_tmp,(sizeof(int)*tmp_size));
//                    printf("STOP DRIVER READING KBytes/Sec %f\n",((sizeof(int)*tmp_size*1000)/time_tmp));
                
                    usleep(loop_delay);
                
                }
                printf ("PRINT (0 NO, 1 YES)  -\n");
                scanf ("%d",&tmp_print);
                fflush(stdin);
                while (tmp_print){
                    printf ("START POS  -\n");
                    scanf ("%d",&tmp_print_start);
                    fflush(stdin);
                    printf ("STOP POS  -\n");
                    scanf ("%d",&tmp_print_stop);
                    fflush(stdin);
                    k = tmp_print_start*4;
                    for(int i = tmp_print_start; i < tmp_print_stop; i++){
                            printf("NUM %i OFFSET %X : DATA %X\n", i,k, (u_int)(tmp_dma_buf[i] & 0xFFFFFFFF));
                            k += 4;
                    }
                    printf ("PRINT (0 NO, 1 YES)  -\n");
		    scanf ("%d",&tmp_print);
		    fflush(stdin);
                }
                if(tmp_dma_buf) delete tmp_dma_buf;
                break;
	   default:
		break;
	}
    }

    close(fd);
    return 0;
}

