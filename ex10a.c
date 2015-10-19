#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/intr.h>
#include <sys/io.h>
#include <native/task.h>

const RTIME period = 1e5;
const int nsamples = 10000;
	
RT_TASK task;
RTIME w_time, Time[10000], time_diff[10000];

void write_RTIMES(char * filename, unsigned int number_of_values, RTIME *time_values)
{
	unsigned int n=0;
	FILE *file;
	file = fopen(filename,"w");
	while (n<number_of_values) 
	{
	  fprintf(file,"%u;%llu\n",n,time_values[n]);  
	  n++;
	}
	fclose(file);
 }
				  
void do_task(void *arg)
{
	RT_TASK *curtask;
	RT_TASK_INFO curtaskinfo;
	int i;

	// inquire current task
	curtask=rt_task_self();
	rt_task_inquire(curtask,&curtaskinfo);
	
	rt_task_set_periodic(NULL, TM_NOW, period);
	
	for(i=0; i<nsamples; i++)
	{
		w_time = rt_timer_read();
		Time[i] = w_time;
		
		rt_task_wait_period(NULL);
		
		time_diff[i] = rt_timer_read() - w_time;
	}
	write_RTIMES("time_difference.csv",nsamples,time_diff);

	for(i=0; i<nsamples; i++)
	{
	printf("time difference: %d\n", time_diff[i]);	
	}
		
}

//startup code
void startup()
{
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
}
