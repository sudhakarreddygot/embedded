#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/intr.h>
#include <sys/io.h>
#include <native/task.h>

const RTIME period = 100000;
const int nsamples = 10000;
	
RT_TASK task;
RTIME w_time;
RTIME time_diff[10000];

RT_INTR intr;
#define PARPORT_IRQ 7
unsigned char byte;

void enable_interupt()
{
    ioperm(0x37A, 1, 1);
    byte = inb(0x37A);
    byte = byte | 0x10; /* hex 10 = 00010000 */
    outb(byte, 0x37A);

	// enable port D0
	ioperm(0x378, 1, 1);
    byte = inb(0x378);
    byte = byte | 0x01; /* hex 10 = 00010000 */
    outb(byte, 0x378);
}

void disable_interupt()
{
    byte = inb(0x37A);
    byte = byte & 0xEF; /* hex EF = binary 11101111 */
    outb(byte, 0x37A);
}

void write_RTIMES(char * filename, unsigned int number_of_values,
                  RTIME *time_values){
	unsigned int n=0;
	FILE *file;
	file = fopen(filename,"w");
	while (n<number_of_values) {
	  fprintf(file,"%u,%llu\n",n,time_values[n]);  
	  n++;
	}
	fclose(file);
	rt_printf("File saved\n");
 }

void send_parallel_port_intrp()
{
	outb(inb(0x378) & 0xFE, 0x378);
	outb(inb(0x378) | 0x01, 0x378); /* enable interrupt */
}

void do_task(void *arg)
{
	int i;
	
	rt_task_set_periodic(NULL, TM_NOW, period);
	
	for(i=0; i<nsamples; i++)
	{
		w_time = rt_timer_read();
		
		send_parallel_port_intrp();
		
		rt_intr_wait(&intr,TM_INFINITE);
		
		time_diff[i] = rt_timer_read() - w_time;
		
		rt_task_wait_period(NULL);
	}
	write_RTIMES("time_diff_d.csv",nsamples,time_diff);
}

//startup code
void startup()
{
	rt_intr_create(&intr, NULL, PARPORT_IRQ, I_PROPAGATE);
    enable_interupt();
	
	rt_task_create(&task, NULL,0,50,0);
	rt_task_start(&task, &do_task, NULL);
}

void init_xenomai() {
  /* Avoids memory swapping for this program */
  mlockall(MCL_CURRENT|MCL_FUTURE);

  /* Perform auto-init of rt_print buffers if the task doesn't do so */
  rt_print_auto_init(1);
}
int main(int argc, char* argv[])
{
  printf("\nType CTRL-C to end this program\n\n" );

  // code to set things to run xenomai
  init_xenomai();

  //startup code
  startup();

  // wait for CTRL-c is typed to end the program
  pause();
  
  disable_interupt();
}