#define _GNU_SOURCE  
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h> 
#include <err.h>
#include <errno.h>
#include<sys/time.h>
pthread_barrier_t barrier;

typedef struct {
    pthread_t thread_id;
    int thread_num;
    int sched_policy;
    int sched_priority;
    float time_wait;
} thread_info_t;


void *thread_func(void *arg)
{	

    thread_info_t *data = (thread_info_t*)arg;
    pthread_barrier_wait(&barrier);
    float time_waiting = data->time_wait;
    int taskid = data->thread_id;	

    /* 1. Wait until all threads are ready */
   
    //pthread_barrier_wait(&barrier); 	
    /* 2. Do the task */ 
    for (int i = 0; i < 3; i++) {

        // Busy for <time_wait> seconds 
        printf("Thread %d is running\n", taskid);
	struct timeval start;
	struct timeval end;
	double start_time, end_time;
	gettimeofday(&start,NULL);
	start_time = (start.tv_sec*1000000+(double)start.tv_usec)/1000000;
	//printf("start_time: %lf\n", start_time);
	while(1)
	{
	gettimeofday(&end,NULL);
	end_time = (end.tv_sec*1000000+(double)end.tv_usec)/1000000;
	//printf("start_time: %lf\n", start_time);
	if (end_time > start_time+time_waiting);
	    break;
	}
	sched_yield();
    }
    /* 3. Exit the function  */
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // 1. Parse program arguments using getopt
    int num_threads = 0;
    double time_wait = 0.0;
    char *token = NULL;
    char *s1 = NULL;
    char pthread_policy[10][10] ;
    char *s2 = NULL;
    int priorities[10];
    int opt;
	
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch (opt) {
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 't':
                time_wait = atof(optarg);
                break;
            case 's':
                s1 = strdup(optarg);
                token = strtok(s1, ",");
                int i = 0;
                
                while(token!=NULL){
                    strcpy(pthread_policy[i],token);
                    //printf("policy : %s\n", pthread_policy[i]);
                    token =strtok(NULL,",");
                    i++;
                }
                break;
            case 'p':
            	
                s2 = strdup(optarg);
                token = strtok(s2,",");
                int j = 0;
                while(token!=NULL){
                    priorities[j] = atoi(token);
                    //printf("priority : %d\n", priorities[i]);
                    token =strtok(NULL,",");
                    j++;
                }
                break;
            default:
                return 1;
        }
    }
     /* 2. Create <num_threads> worker threads */
    thread_info_t thread_data_array[num_threads];
    pthread_t child_thread_id[num_threads];
    struct sched_param param[num_threads];
    pthread_attr_t attr[num_threads];
   

     /* 3. Set CPU affinity */
    
    int cpu_id = 2; // set thread to cpu2
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    //printf("pthread cpu%d\n",cpu_id);
    pthread_t th =pthread_self();
    pthread_setaffinity_np(th, sizeof(cpuset), &cpuset);
    //printf("sched_getcpu = %d\n",sched_getcpu());

    /* 4. Set the attributes to each thread */
    int policy;
    pthread_barrier_init(&barrier, NULL, num_threads+1);
    for (int i = 0; i < num_threads; i++) {
        thread_data_array[i].thread_id=i;
	thread_data_array[i].time_wait=time_wait;
	pthread_attr_init(&attr[i]); /*initialize thread attribute*/
        pthread_attr_setinheritsched(&attr[i],PTHREAD_EXPLICIT_SCHED); /*set thread inheritance*/
        pthread_attr_getinheritsched(&attr[i],&policy); /*get thread inheritance*/

        
        //other
	if(strcmp(pthread_policy[i],"NORMAL") == 0)
	{
	    pthread_attr_setschedpolicy(&attr[i], SCHED_OTHER);
	    
	}
	else //FIFO
	{
	    pthread_attr_setschedpolicy(&attr[i], SCHED_FIFO);

	}

	param[i].sched_priority=priorities[i];
	if(priorities[i]!=-1)
	{
	    pthread_attr_setschedparam(&attr[i],&param[i]);/*Set thread scheduling parameters*/
	}
	pthread_create(&child_thread_id[i], &attr[i], thread_func, (void *)
	&thread_data_array[i]);
	
    }
    
    // 5. Start all threads at once */

    pthread_barrier_wait(&barrier);
    for (int i = 0; i < num_threads; i++) {
    pthread_join(child_thread_id[i], NULL);
}
    pthread_barrier_destroy(&barrier); 
    return 0;
}
