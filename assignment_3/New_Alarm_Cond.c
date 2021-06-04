/*
 * alarm_cond.c
 *
 * This is an enhancement to the alarm_mutex.c program, which
 * used only a mutex to synchronize access to the shared alarm
 * list. This version adds a condition variable. The alarm
 * thread waits on this condition variable, with a timeout that
 * corresponds to the earliest timer request. If the main thread
 * enters an earlier timeout, it signals the condition variable
 * so that the alarm thread will wake up and process the earlier
 * timeout first, requeueing the later request.
 */
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include "errors.h"

#define DEBUG

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
    char                message[128];
	int 				num;
	int					isCancel;
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t alarm_cond = PTHREAD_COND_INITIALIZER;
alarm_t *alarm_list = NULL;
time_t current_alarm = 0;
sem_t sem_w;
sem_t sem_r;
int read_counter = 0;//may not need



void *periodic_display_thread(void *arg)
{
	
}
/*
 * Insert alarm entry on list, in order.
 */
void alarm_insert (alarm_t *alarm)
{
    int status;
    alarm_t **last, *next;

    /*
     * LOCKING PROTOCOL:
     * 
     * This routine requires that the caller have locked the
     * alarm_mutex!
     */
    last = &alarm_list;
    next = *last;
    while (next != NULL) {
        if (next->num > alarm->num) {
            alarm->link = next;
            *last = alarm;
            break;
        }
		else if (next->num == alarm->num)
		{
			alarm->link = next->link;
			*last = alarm;
			break;
		}
        last = &next->link;
        next = next->link;
    }
    /*
     * If we reached the end of the list, insert the new alarm
     * there.  ("next" is NULL, and "last" points to the link
     * field of the last item, or to the list header.)
     */
    if (next == NULL) {
        *last = alarm;
        alarm->link = NULL;
    }
#ifdef DEBUG
    printf ("[list: ");
    for (next = alarm_list; next != NULL; next = next->link)
        printf ("%d(%d)[\"%s\"] ", next->time,
            next->time - time (NULL), next->message);
    printf ("]\n");
#endif
    /*
     * Wake the alarm thread if it is not busy (that is, if
     * current_alarm is 0, signifying that it's waiting for
     * work), or if the new alarm comes before the one on
     * which the alarm thread is waiting.
     *
    if (current_alarm == 0 || alarm->time < current_alarm) {
        current_alarm = alarm->time;
        status = pthread_cond_signal (&alarm_cond);
        if (status != 0)
            err_abort (status, "Signal cond");
    }*/
	//wake after insert
	status = pthread_cond_signal(&alarm_cond);
	if (status != 0)
		err_abort(status, "Signal cond");
}

/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    struct timespec cond_time;
    time_t now;
    int status, expired;
	pthread_t display_thread;

    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits. Lock the mutex
     * at the start -- it will be unlocked during condition
     * waits, so the main thread can insert alarms.
     */
    

	//start read
    while (1) {
		//status = pthread_mutex_lock (&alarm_mutex);
		status = sem_wait(&sem_r);//to read
		read_counter++;
		if (read_counter == 1)
		{
			sem_wait(&sem_w);
		}
		sem_post(&sem_r);
		/*
         * If the alarm list is empty, wait until an alarm is
         * added. Setting current_alarm to 0 informs the insert
         * routine that the thread is not busy.
         */
        current_alarm = 0;
        //while (alarm_list == NULL) {
            status = pthread_cond_wait (&alarm_cond, &alarm_mutex);
            if (status != 0)
                err_abort (status, "Wait on cond");
            }
		//get new element?
        alarm = alarm_list;//first element
		//done reading
		sem_wait(&sem_r);
		read_counter--;

		if (read_counter == 0)
		{
			sem_post(&sem_w);
		}
		sem_post(&sem_r);

		//write
		sem_wait(&sem_w);

		//remove first element
		alarm_list = alarm->link;

		//printf("iscanc: %d, num: %d, mes:%s", alarm->isCancel, alarm->num, alarm->message);
		if (alarm->isCancel == 0)//normal request
		{
			//create display thread upon new list entry
			status = pthread_create(
				&display_thread, NULL, periodic_display_thread, NULL);
			if (status != 0)
				err_abort(status, "Create alarm thread");
			printf("DISPLAY THREAD CREATED FOR : Message(%d) %s\n", alarm->num, alarm->message);
		}
		else//cancel message
		{

			//find alarm to cancel
			alarm_t *next, *last;
			next = alarm_list;
			last = NULL;
			while (next != NULL)
			{
				if (!next->isCancel && next->num == alarm->num)
				{
					//cancel
					//remove from list
					if (last == NULL)
					{
						alarm_list = next->link;
					}
					else
					{
						last->link = next->link;
					}
					printf("CANCEL: Message(%d) %s", alarm->num, next->message);
					break;
				}
				last = next;
				next = next->link;
			}

		}
		//done writing
		sem_post(&sem_w);

        now = time (NULL);
		expired = 0;

        if (alarm->time > now) {
#ifdef DEBUG
            printf ("[waiting: %d(%d)\"%s\"]\n", alarm->time,
                alarm->time - time (NULL), alarm->message);
#endif
            cond_time.tv_sec = alarm->time;
            cond_time.tv_nsec = 0;
            current_alarm = alarm->time;
            while (current_alarm == alarm->time) {
                status = pthread_cond_timedwait (
                    &alarm_cond, &alarm_mutex, &cond_time);
                if (status == ETIMEDOUT) {
                    expired = 1;
                    break;
                }
                if (status != 0)
                    err_abort (status, "Cond timedwait");
            }
            if (!expired)
                alarm_insert (alarm);
        } else
            expired = 1;
        if (expired) {
            printf ("(%d) %s\n", alarm->seconds, alarm->message);
            free (alarm);
        }
    }
}

int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm;
    pthread_t thread;
	int good_input = 0;

	sem_init(&sem_w, 1, 1);//create writer semaphore
	sem_init(&sem_r, 1, 1);//create reader semaphore for mutual exclusion

    status = pthread_create (
        &thread, NULL, alarm_thread, NULL);
    if (status != 0)
        err_abort (status, "Create alarm thread");
    while (1) {
        printf ("Alarm> ");
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
		if (sscanf(line, "%d Message(%d) %128[^\n]", &alarm->seconds, &alarm->num, alarm->message) == 3)
		{
			
			alarm->isCancel = 0;
			good_input = 1;
		}
		else if (sscanf(line, "Cancel: Message(%d)[^\n]", &alarm->num) == 1)
		{
			alarm->isCancel = 1;
			good_input = 1;
		}
		else
		{
			fprintf(stderr, "Bad command\n");
			free(alarm);
			good_input = 0;
		}

		if(good_input)
		{
			//add to list
            //status = pthread_mutex_lock (&alarm_mutex);
			status = sem_wait(&sem_w);//wait
			if (status != 0)
                err_abort (status, "Lock mutex");

            alarm->time = time (NULL) + alarm->seconds; //time of expiry
            /*
             * Insert the new alarm into the list of alarms,
             * sorted by message number.
             */
            alarm_insert (alarm);
            //status = pthread_mutex_unlock (&alarm_mutex);
			status = sem_post(&sem_w);//signal
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
    }
}
