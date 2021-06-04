/*
 alarm_mutex.c
  
 Daniel Kozlovsky, 214874879
 Jada Jones, 215562044
 Ahaz Goraya, 215117971
 
 EECS 3221E Assignment 2
 November 12, 2019
 
 A modification of alarm_mutex.c that uses two extra parallel
 threads to manage odd and even requests, thus dividing the work. 
 Alarms are managed on an expiration basis.  
 */
 
 
#include <pthread.h>
#include <time.h>
#include "errors.h"
#include <stdint.h>
//#define DEBUG

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[64];
	int 				request_num ;
} alarm_t;
/* one mutex for each list because they may be accessed at different 
times. This way is faster and more efficient */
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t even_alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t odd_alarm_mutex = PTHREAD_MUTEX_INITIALIZER;

alarm_t *alarm_list = NULL; /*intermediate storage for new alarm requests */
alarm_t *even_alarm_list = NULL; /*even numbered requests */
alarm_t *odd_alarm_list = NULL; /* odd numbered requests */


/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
	alarm_t *current_alarm;
    alarm_t *alarm;
	alarm_t *even_alarm, *odd_alarm;
    int sleep_time;
    time_t now;
    int status;
	
	
	/*Make new even_alarm*/
	even_alarm = (alarm_t*)malloc (sizeof (alarm_t));
       if (even_alarm == NULL)
           errno_abort ("Allocate even alarm");
	/*Make new odd_alarm*/
	odd_alarm = (alarm_t*)malloc (sizeof (alarm_t));
       if (odd_alarm == NULL)
           errno_abort ("Allocate odd alarm");
    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) {
		
		/* lock main alarm list to prevent adding removing while it's processed */
        status = pthread_mutex_lock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");
        alarm = alarm_list; //get first item
		//make an alarm copy so as not to interfere with the list
		current_alarm = NULL;
		if(alarm != NULL)
		{
			current_alarm = (alarm_t*)malloc (sizeof (alarm_t));
			if (current_alarm == NULL)
				errno_abort ("Allocate even alarm");
			memcpy(current_alarm, alarm, sizeof alarm);
			current_alarm->link = NULL; //should not have link for this list
			current_alarm->seconds = alarm->seconds;
			current_alarm->time = alarm->time;
			strcpy(current_alarm->message, alarm->message);
			current_alarm->request_num = alarm->request_num;
		}
		
        /*
         * If the alarm list is empty, wait for one second. This
         * allows the main thread to run, and read another
         * command. If the list is not empty, remove the first
         * item. Compute the number of seconds to wait -- if the
         * result is less than 0 (the time has passed), then set
         * the sleep_time to 0.
         */
        if (current_alarm == NULL)
            sleep_time = 1;
        else {
			if(current_alarm->request_num % 2 == 1)/* request is odd*/
			{
				alarm_t *last, *next;
				
				pthread_mutex_lock(&odd_alarm_mutex);
				/*
				* Insert the new alarm into the list of odd alarms,
				* sorted by expiration time.
				*/
				last = NULL;
				next = odd_alarm_list;
				if(next == NULL)//first entry into list
				{
					odd_alarm_list = alarm;
					current_alarm->link = NULL;
				}
				else
				{
					while(next != NULL)//iterate through list
					{
						if(current_alarm->time <= next->time)
						{
							current_alarm->link = next; //insert in between items
							if(last != NULL)
								last->link = current_alarm;
							else
								odd_alarm_list = current_alarm;
							
							break;
						}
					last = next;
					next = next->link; //iterate
					
					}
					if(next == NULL)
					{
						last->link = current_alarm;
					}
				}
				
				pthread_mutex_unlock(&odd_alarm_mutex); /*unlock -- done with list */
				fprintf(stdout,"Alarm Thread Passed on Alarm Request to Display Thread 1 Alarm Request Number:(%d) Alarm Request: (%d) [\"%s\"]\n",
					current_alarm->request_num, current_alarm->seconds, current_alarm->message);
				
				fprintf(stdout,"Display Thread 1: Received Alarm Request Number:%d Alarm Request: (%d) [\"%s\"]\n",
					current_alarm->request_num, current_alarm->seconds, current_alarm->message);
				
			}
			else /* request is even*/
			{
				alarm_t *last, *next;
				
				pthread_mutex_lock(&even_alarm_mutex);
				/*
				* Insert the new alarm into the list of alarms,
				* sorted by expiration time.
				*/
				last = NULL;
				next = even_alarm_list;
				if(next == NULL)//first entry into list
				{
					even_alarm_list = current_alarm;
					current_alarm->link = NULL;
				}
				else
				{
					while(next != NULL)//iterate through list
					{
						if(current_alarm->time <= next->time)
						{
							current_alarm->link = next;//insert in between items
							if(last != NULL)
								last->link = current_alarm;
							else
								even_alarm_list = current_alarm;
							
							break;
						}
					last = next;
					next = next->link;//iterate
					
					}
					if(next == NULL)
					{
						last->link = current_alarm;
					}
				}
				pthread_mutex_unlock(&even_alarm_mutex);/*unlock -- done with list */
				fprintf(stdout,"Alarm Thread Passed on Alarm Request to Display Thread 2 Alarm Request Number:(%d) Alarm Request: (%d) [\"%s\"]\n",
					current_alarm->request_num, current_alarm->seconds, current_alarm->message);
					
				fprintf(stdout,"Display Thread 2: Received Alarm Request Number:%d Alarm Request: (%d) [\"%s\"]\n",
					current_alarm->request_num, current_alarm->seconds, current_alarm->message);
			}
			
			/*remove the first item*/
			alarm_list = alarm->link;
			now = time (NULL);
			if (alarm->time <= now)
				sleep_time = 0;
			else
				sleep_time = 1;//alarm->time - now;
            
            }

        /*
         * Unlock the mutex before waiting, so that the main
         * thread can lock it to insert a new alarm request.
         */
        status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");
		//give CPU to main
        sched_yield ();
    }
}
/*display_thread_1 start routine for handling odd requests*/
 /*odd requests*/
void *display_thread_1_r (void *arg)
{
	alarm_t *current_alarm;
	alarm_t *next;
	/*requested amount of seconds */
	int req_seconds;
	/*alarm number */
	int req_num; 
	/*Alarm message */
	int alarm_time;
	/* message */
	char *str;
	
	/* timestamp to keep track of every 2 seconds */
	int prev_timestamp;
	
	while(1)
	{

		if(odd_alarm_list != NULL) /*only if list is not empty */
		{
			/*lock odd alarm mutex so it can be modified without race conditions, etc...*/
			pthread_mutex_lock(&odd_alarm_mutex);
			int now = time (NULL);
			
			
			//new request
			if(current_alarm != odd_alarm_list)//Checking for new request
			{
				/*get first node */
				current_alarm = odd_alarm_list;
				/* running number of seconds left (diminishing)*/
				req_seconds = current_alarm->seconds;
				/*alarm number */
				req_num = current_alarm->request_num;
				/*time of request */
				alarm_time = current_alarm->time;
				/*Alarm message */
				str = current_alarm->message;
				
				/*timestamp to keep track of every two seconds*/
				prev_timestamp = now;
				
				
					
					//DEBUGGING
			#ifdef DEBUG
            printf ("[odd list: ");
            for (next = odd_alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - now, next->message);
            printf ("]\n");
			#endif
			}	
			
			/* Repeat every two seconds while alarm hasn't expired */
			if(current_alarm->time - now > 0 && now - prev_timestamp >= 2)
			{
				fprintf(stdout,"Display Thread 1: Number of Seconds Left %d : Alarm Request Number: (%d) Alarm Request: (%d) [\"%s\"]\n",
					current_alarm->time - now, req_num, req_seconds, str);
				
				/*reset timestamp */
				prev_timestamp = now;
			}
			else if(current_alarm->time - now < 0) //Alarm expired
			{
				fprintf(stdout,"Display Thread 1: Alarm Expired at %d : Alarm Request Number: (%d) Alarm Request: (%d) [\"%s\"]\n",
					time (NULL), req_num, req_seconds, str);
				printf ("(%d) %s\n", req_seconds, str); /*print alarm after expired */
				
				/* remove first node */
				odd_alarm_list = current_alarm->link;
				//free(current_alarm);
			}
			
			/* unlock to allow items to be added to odd list */
				pthread_mutex_unlock(&odd_alarm_mutex);
		}
	}
}

/*display_thread_2 start routine for handling even requests*/
void *display_thread_2_r (void *arg)
{
	alarm_t *current_alarm;
	alarm_t *next;
	/*requested amount of seconds */
	int req_seconds;
	/*alarm number */
	int req_num; 
	/*Alarm message */
	int alarm_time;
	/* message */
	char *str;
	
	/* timestamp to keep track of every 2 seconds */
	int prev_timestamp;
	
	while(1)
	{

		
		if(even_alarm_list != NULL) /*only if list is not empty */
		{
			/*lock odd alarm mutex so it can be modified without race conditions, etc...*/
			pthread_mutex_lock(&even_alarm_mutex);
			int now = time (NULL);
			
			
			//new request
			if(current_alarm != even_alarm_list)//Checking for new request
			{
				/*get first node */
				current_alarm = even_alarm_list;
				/* running number of seconds left (diminishing)*/
				req_seconds = current_alarm->seconds;
				/*alarm number */
				req_num = current_alarm->request_num;
				/*time of request */
				alarm_time = current_alarm->time;
				/*Alarm message */
				str = current_alarm->message;
				
				/*timestamp to keep track of every two seconds*/
				prev_timestamp = now;
				
					
					//DEBUGGING
			#ifdef DEBUG
            printf ("[even list: ");
            for (next = even_alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - now, next->message);
            printf ("]\n");
			#endif
			}	
			
			/* Repeat every two seconds while alarm hasn't expired */
			if(current_alarm->time - now > 0 && now - prev_timestamp >= 2)
			{
				fprintf(stdout,"Display Thread 2: Number of Seconds Left %d : Alarm Request Number: (%d) Alarm Request: (%d) [\"%s\"]\n",
					current_alarm->time - now, req_num, req_seconds, str);
				
				/*reset timestamp */
				prev_timestamp = now;
			}
			else if(current_alarm->time - now < 0) //Alarm expired
			{
				fprintf(stdout,"Display Thread 2: Alarm Expired at %d : Alarm Request Number: (%d) Alarm Request: (%d) [\"%s\"]\n",
					time (NULL), req_num, req_seconds, str);
				printf ("(%d) %s\n", req_seconds, str); /*print alarm after expired */
				
				/* remove first node */
				even_alarm_list = current_alarm->link;
				//free(current_alarm);
			}
			
			/* unlock to allow items to be added to odd list */
				pthread_mutex_unlock(&even_alarm_mutex);
		}
	}
}
int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm, *last, *next;
    pthread_t thread;
	/* display threads */
	pthread_t display_thread_1, display_thread_2;
	/* number of requested alarm, positive*/
	uint32_t Alarm_Request_Number = 0;
	
	/*new alarm thread*/
    status = pthread_create (
        &thread, NULL, alarm_thread, NULL);
    if (status != 0)
        err_abort (status, "Create alarm thread");
	/*new display thread 1*/
	status = pthread_create (
        &display_thread_1, NULL, display_thread_1_r, NULL);
    if (status != 0)
        err_abort (status, "Create display_thread_1");
	/*new display thread 2*/
	status = pthread_create (
        &display_thread_2, NULL, display_thread_2_r, NULL);
    if (status != 0)
        err_abort (status, "Create display_thread_2");
	
	/* Main loop */
    while (1) {
		//User input
        printf ("alarm> \n");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */
        if (sscanf (line, "%d %64[^\n]", 
            &alarm->seconds, alarm->message) < 2 || alarm->seconds < 0) {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        } else {
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;
			
			Alarm_Request_Number++; /*increment alarm request counter*/
			alarm->request_num = Alarm_Request_Number; /*Set request number of the current alarm*/
			fprintf(stdout, "Main Thread Received Alarm Request Number:(%d) Alarm Request: (%d) [\"%s\"]\n",
				Alarm_Request_Number, alarm->seconds, alarm->message );
			
            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
			last = NULL;
			next = alarm_list;
			if(next == NULL)//first entry into list
			{
				alarm_list = alarm;
				alarm->link = NULL;
			}
			else
			{
				while(next != NULL)//iterate through list
				{
					//insert before smallest time that's still larger than alarm.time
					if(alarm->time <= next->time)
					{
						alarm->link = next;
						if(last != NULL)
							last->link = alarm;
						else
							alarm_list = alarm;
						
						break;
					}
				last = next;
				next = next->link;//iterate
				
				}
				if(next == NULL)
				{
					last->link = alarm;
				}
			}
			
			
#ifdef DEBUG
            printf ("[list: ");
            for (next = alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - time (NULL), next->message);
            printf ("]\n");
#endif
#ifdef DEBUG
            printf ("[odd list: ");
            for (next = odd_alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - time (NULL), next->message);
            printf ("]\n");
			#endif
			#ifdef DEBUG
            printf ("[even list: ");
            for (next = even_alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - time (NULL), next->message);
            printf ("]\n");
			#endif
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
    }
}
