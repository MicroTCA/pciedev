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
#include "x1timer_io.h"

/* useconds from struct timeval */
#define	MIKRS(tv) (((double)(tv).tv_usec ) + ((double)(tv).tv_sec * 1000000.0)) 
#define	MILLS(tv) (((double)(tv).tv_usec/1000 )  + ((double)(tv).tv_sec * 1000.0)) 

int	         fd;
int	         fd_timer;

struct timeval          p_time_s;
struct timeval          p_time_e;
double                time_tmp = 0;
double                time_dlt;

struct sig_event  {
       time_t sig_time_s;
       time_t sig_time_u;
       time_t sig_time_old_s;
       time_t sig_time_old_u;
       time_t drv_sig_time_new_s;
       time_t drv_sig_time_new_u;
       time_t drv_sig_time_old_s;
       time_t drv_sig_time_old_u;
       u_int		sig_mask;
       u_int		sig_flag;
};

int main(int argc, char* argv[])
{
    int  i = 0;
    char nod_name[15] = "";
    char timer_name[15] = "";
    device_rw	          l_Read;
    int			  tmp_mode;
    u_int	          tmp_offset;
    int      	          tmp_data;
    int      	          tmp_barx;
    int                n = 0;
    int                k;
    int                odd = 0;
    int                mask_sts = 0;
    int	                tmp_cnt          = 0;
    int                was_matched = 0;
    int                sig;
    sigset_t           mask;
    sigset_t           w_mask;
    int                code = 0;
    u_int              tmp_mask = 0x1;
    t_x1timer_ioctl_data	l_Ctrl;
    t_x1timer_ioctl_event   Event_Ctrl;

    int                   len = 0;
    int                   itemsize = 0;
	
    struct sig_event      sig_event[4];

    itemsize = sizeof(device_rw);

    l_Ctrl.f_Irq_Mask  = 0;
    l_Ctrl.f_Irq_Sig   = 0;
    l_Ctrl.f_Status    = 0;

    if(argc <=1){
        printf("Input \"prog /dev/pciedevs4 /dev/x1timerX\" \n");
        return 0;
    }
    strncpy(nod_name,argv[1],sizeof(nod_name));
    fd = open (nod_name, O_RDWR);
    if (fd < 0) {
        printf ("#CAN'T OPEN PCIEDEV FILE \n");
        exit (1);
    }
   

    strncpy(timer_name,argv[2],sizeof(timer_name));
    fd_timer = open (timer_name, O_RDWR);
    if (fd_timer < 0) {
        printf ("#CAN'T OPEN XTIMER FILE \n");
        exit (1);
    }

    printf ("INPUT IRQ MASK (HEX)  -\n");
    scanf ("%x",&(sig_event[i].sig_mask));
    fflush(stdin);
    
    sigemptyset (&mask);
    sigaddset   (&mask, SIGUSR1);
    sigprocmask (SIG_BLOCK, &mask, NULL);
    
    l_Read.offset_rw   = 0;
    l_Read.data_rw     = 0;
    l_Read.mode_rw     = 0;
    l_Read.barx_rw     = 0;
    l_Read.size_rw     = 0;
    l_Read.rsrvd_rw    = 0;
    printf ("\n INPUT  BARx (0,1,2,3...)  -");
    scanf ("%x",&tmp_barx);
    fflush(stdin);

    printf ("\n INPUT  MODE  (0-D8,1-D16,2-D32)  -");
    scanf ("%x",&tmp_mode);
    fflush(stdin);

    printf ("\n INPUT  ADDRESS (IN HEX)  -");
    scanf ("%x",&tmp_offset);
    fflush(stdin);

    printf ("\n INPUT POLING RATE IN us  -");
    scanf ("%i",&tmp_data);
    fflush(stdin);
    tmp_cnt          = 90000/tmp_data;

    tmp_mask |= sig_event[i].sig_mask;
    l_Ctrl.f_Irq_Mask	= (tmp_mask & 0XFFFFFFFF);
    l_Ctrl.f_Irq_Sig	= SIG1;
    l_Ctrl.f_Status     = 0;
    code = ioctl (fd_timer, X1TIMER_ADD_SERVER_SIGNAL, &l_Ctrl);
    if (code) {
        printf ("Error X1IMER_ADD_SERVER_SIGNAL - %i\n", fd);
            printf ("#2 %d\n", code);
    }
    sigemptyset (&w_mask);
    sigaddset   (&w_mask, SIGUSR1);
    sigaddset   (&w_mask, SIGUSR2);   
        
    while (1) {
        sigwait(&w_mask, &sig);
		
        gettimeofday (&p_time_e, NULL);
        printf ("START POLL TIME      %i:%i\n", p_time_e.tv_sec, p_time_e.tv_usec);
        tmp_cnt          = 90000/tmp_data;
        while (tmp_cnt){
            l_Read.data_rw   = 0;
            l_Read.offset_rw = tmp_offset;
            l_Read.mode_rw   = tmp_mode;
            l_Read.barx_rw   = tmp_barx;
            l_Read.size_rw   = 0;
            l_Read.rsrvd_rw  = 0;
            gettimeofday (&p_time_s, NULL);
            len = read (fd, &l_Read, sizeof(device_rw));
            if (len != itemsize ){
               printf ("#CAN'T READ FILE ERROR %i\n", len);
               close(fd);
               return 0;
            }
            time_tmp = MIKRS(p_time_e) - MIKRS(p_time_s);
            time_dlt    = MILLS(p_time_e) - MILLS(p_time_s);
            p_time_s.tv_usec  = p_time_e.tv_usec ;
            p_time_s.tv_sec    = p_time_e.tv_sec;
            if(l_Read.data_rw){
                printf ("########################################READED : DATA - %X  OFFSET  %X TIME %fms : %fmks \n", l_Read.data_rw, l_Read.offset_rw, time_dlt, time_tmp);
            }else{
                printf ("READED : DATA - %X  OFFSET  %X TIME %fms : %fmks \n", l_Read.data_rw, l_Read.offset_rw, time_dlt, time_tmp);
            }
            
            usleep(tmp_data);
            tmp_cnt-- ;
        }
    }    
    close(fd_timer);
    close(fd);
    return 0;
}

