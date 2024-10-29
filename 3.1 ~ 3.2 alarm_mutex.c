/
 * This alarm_mutex.c  At present, it is basically operational
 * At present, 3.1 to 3.2 have been completed. Except 3.2.5!!!!(Since I haven't created the display thread yet, I can only do a simple view first. Try looking at the display)
 * Important thing: Since I failed to find the bug that the alarm triggered together, I shelved the issue, if you can fix it, or if you are experiencing a more serious bug, please contact at discord
 *
 *
 *
 * This is an enhancement to the alarm_thread.c program, which
 * created an "alarm thread" for each alarm command. This new
 * version uses a single alarm thread, which reads the next
 * entry in a list. The main thread places new requests onto the
 * list, in order of absolute expiration time. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 */
#include <pthread.h>
#include <time.h>
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALARMS_PER_THREAD 2  // Maximum number of alarms a display thread can handle


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
    int                 Alarm_ID;//Ryan: ADDED for ID
    char                Type[10];//Ryan: ADDED for Type
    int                 expired;   // 0 = active, 1 = expired
    int                 cancelled; // 0 = active, 1 = cancelled
} alarm_t;

// Define display thread structure
typedef struct display_thread_tag {
    pthread_t           thread_id;
    char                alarm_type[10];
    int                 alarm_count;
    alarm_t             *alarms[MAX_ALARMS_PER_THREAD];
} display_thread_t;


// Mutex for alarm list and display thread management
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;


// Array to manage display threads
display_thread_t *display_threads[100];
int display_thread_count = 0;
 

// Function prototypes
void *alarm_thread(void *arg);
void *display_thread(void *arg);


// Function to create a new display thread for a specific alarm type
void create_display_thread(const char *type, alarm_t *alarm) {
    pthread_t thread;
    display_thread_t *new_thread = (display_thread_t *)malloc(sizeof(display_thread_t));
    if (new_thread == NULL) {
        perror("malloc failed");
        exit(1);
    }

    strcpy(new_thread->alarm_type, type);
    new_thread->alarm_count = 0;
    new_thread->alarms[new_thread->alarm_count++] = alarm;
    new_thread->thread_id = thread;

    // Create the new display thread
    int status = pthread_create(&thread, NULL, display_thread, (void *)new_thread);
    if (status != 0) {
        err_abort(status, "Create display thread");
    }

    // Add the new display thread to the array
    pthread_mutex_lock(&alarm_mutex);
    display_threads[display_thread_count++] = new_thread;
    pthread_mutex_unlock(&alarm_mutex);

    // Print the creation message
    time_t now = time(NULL);
    printf("First New Display Thread(%lu) Created at %ld: %s %d %s\n", 
           (unsigned long)thread, now, alarm->Type, alarm->seconds, alarm->message);
}


// Function to assign an alarm to a suitable display thread
void assign_alarm_to_display_thread(alarm_t *alarm) {
    pthread_mutex_lock(&alarm_mutex);

    // Check if a display thread exists for the alarm type and if it has space
    for (int i = 0; i < display_thread_count; i++) {
        display_thread_t *dt = display_threads[i];

        // Check if this thread handles the alarm type and has space
        if (strcmp(dt->alarm_type, alarm->Type) == 0 && dt->alarm_count < MAX_ALARMS_PER_THREAD) {
            dt->alarms[dt->alarm_count++] = alarm;
            time_t now = time(NULL);

            // Print assignment message
            printf("Alarm (%d) Assigned to Display Thread(%lu) at %ld: %s %d %s\n", 
                   alarm->Alarm_ID, (unsigned long)dt->thread_id, now, alarm->Type, alarm->seconds, alarm->message);

            pthread_mutex_unlock(&alarm_mutex);
            return;
        }
    }

    // If no suitable thread was found, create a new one
    pthread_mutex_unlock(&alarm_mutex);
    create_display_thread(alarm->Type, alarm);
}



// Display thread function placeholder
void *display_thread(void *arg) {
    display_thread_t *thread_info = (display_thread_t *)arg;
    // Implement logic for handling alarms in this display thread
    // This will be where the specific alarm messages get processed
    // based on their type and timing.
    return NULL;
}




//tester to see the entire list
void print_alarm_list() {
    alarm_t *current = alarm_list;
    if (current == NULL) {
        printf("No alarms in the list\n");
        return;
    }
    printf("Current Alarms in the List:\n");
    while (current != NULL) {
        printf("Alarm_ID: %d, Type: %s, Seconds: %d, Message: %s, Time: %ld\n", 
               current->Alarm_ID, current->Type, current->seconds, current->message, current->time);
        current = current->link; // Move to the next alarm in the list
    }
}

/*
 * The alarm thread's start routine.
 */
/*void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    int sleep_time;
    time_t now;
    int status;


    while (1) {
        status = pthread_mutex_lock (&alarm_mutex);//to lock
        if (status != 0)//if status is not locked, which status will not equal to 1, then print error message
            err_abort (status, "Lock mutex");//the system would print the error message and then terminate
        alarm = alarm_list;//the thread assigns the first alarm from the alarm_list to the alarm pointer. so pointer(alarm) is pointing to the 
                        //current alarm to be processed


        if (alarm == NULL)//if there is no alarm in the list
            sleep_time = 1;//sleep for 1 second
        else {
            alarm_list = alarm->link;//the current alarm is removed and the head of the list is pointing to next alarm in the list
            now = time (NULL);//record current time
            if (alarm->time <= now)//if the alarm's trigger time has already passed or is due
                sleep_time = 0;//don't sleep, run it immediately 
            else
                sleep_time = alarm->time - now;//if the alarm is not passed or due, then the thread get sleep_time
#ifdef DEBUG//if the program is in debug mode, this block will print debugging info like the alarm's time and the calculated sleep time and message
            printf ("[waiting: %d(%d)\"%s\"]\n", alarm->time,
                sleep_time, alarm->message);
#endif
            }


        status = pthread_mutex_unlock (&alarm_mutex);//once the thread finished working with the alarm list, it unlocks the mutex
        if (status != 0)//see if the unlock is working or not
            err_abort (status, "Unlock mutex");//system print an error message and then terminate the program
        if (sleep_time > 0)
            sleep (sleep_time);
        else//sleep_time == 0
            sched_yield ();//there is no time to sleep and sched_yield() can ask another thread to run immediately 


        if (alarm != NULL) {//print the alarm message and free the memory 
            printf ("(%d) %s\n", alarm->seconds, alarm->message);
            free (alarm);
        }
    }
}*/
void *alarm_thread(void *arg)
{
    alarm_t *alarm;
    int sleep_time;    
    time_t now;  
    int status;

    while (1) {
        // Lock the mutex to safely access the shared alarm list
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Lock mutex");

        if (alarm_list == NULL) {
            // If the alarm list is empty, set sleep_time to 1 second
            sleep_time = 1;
        } else {
            // Calculate the time until the next alarm is due
            now = time(NULL);  // Get the current time
            sleep_time = alarm_list->time - now;  // Time until the next alarm
            if (sleep_time <= 0)
                sleep_time = 0;  // if the alarm is due or overdue, set sleep_time to 0
        }

        // Unlock the mutex before sleeping to allow other threads to access the alarm list
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Unlock mutex");

        // Sleep for the calculated time
        if (sleep_time > 0)
            sleep(sleep_time);
        else
            sched_yield();  

        // After waking up, lock the mutex again to check for due alarms
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Lock mutex");

        now = time(NULL); // Update the current time

        // Process all alarms that are due
        while (alarm_list != NULL && alarm_list->time <= now) {
            // Remove the alarm from the list and process it
            alarm_t *alarm = alarm_list; // Get the first alarm in the list
            alarm_list = alarm_list->link; // Remove it from the list

            // Unlock the mutex before processing the alarm
            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock mutex");

            // Print the alarm message
            printf("(%d) %s\n", alarm->Alarm_ID, alarm->message);

            // Print the expiration message
            printf("Alarm(%d): Alarm Expired at %ld: Alarm Removed From Alarm List\n", 
                   alarm->Alarm_ID, now);


            free(alarm);        // Free the memory allocated for the alarm
            printf("alarm> ");  // Print the prompt immediately after the alarm message
            fflush(stdout);     // Flush the output buffer to make sure it displays immediately

            // Re-lock the mutex to check if there are more due alarms
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            now = time(NULL); // Update the current time
        }

        // Unlock the mutex after processing due alarms
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Unlock mutex");
    }
}

int main (int argc, char *argv[])
{
    int status;//returned value of thread-realted and mutex functions like pthread_create() and pthread_mutex_lock(), to check success or not
    char line[128];//user input
    alarm_t *alarm, **last, *next;//*alarm: pointer to alarm_t; **last: point to the next alarm's pointer; *next: insert the new alarm in the correct place
    pthread_t thread;//thread identifier that will be used to create and managed the alarm thread

    status = pthread_create (//create a new thread to run the alarm_thread function
        &thread, NULL, alarm_thread, NULL);//&thread: pointer to the pthread_t object where the thread Id is stored; alarm_thread: the function to be executed by the new thread
    if (status != 0)//if it is success, then status = 0
        err_abort (status, "Create alarm thread");//error message
    while (1) {
        printf ("alarm> ");//display a prompt "alarm>" to user and asking them to enter an alarm
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);//read the info that user entered
        if (strlen (line) <= 1) continue;//double check if the user enter empty line or just press enter. if it is this case, then reprompt, dont create an alarm

        

        alarm = (alarm_t*)malloc (sizeof (alarm_t));//allocates memory for a new alarm_t with size of alarm_t
        if (alarm == NULL)//check if malloc(memory allocation) was successful
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */

        //handle start_alarm case
        if (strncmp(line, "Start_Alarm", 11) == 0) {           
            //check if user enter Alarm_ID, Type,seconds, message
            if (sscanf (line, "Start_Alarm(%d): %s %d %128[^\n]", &alarm->Alarm_ID, alarm->Type, &alarm->seconds, alarm->message) < 4) {//check if user enter both valid number of seconds and a message, if user didnt provide both, then it is bad
                fprintf (stderr, "Bad command\n");
                print_alarm_list(); // print the list(just for debugging)
                free (alarm);
            } else {
                status = pthread_mutex_lock (&alarm_mutex);//lock alarm_mutex and no other thread can modify the alarm_list while we insert new alarm
                if (status != 0)//success lock or not
                    err_abort (status, "Lock mutex");
                alarm->time = time (NULL) + alarm->seconds;//alarm->time calculate the current time(since Unix Epoch) + the user entered time

                // Truncate message to 128 characters
                alarm->message[127] = '\0';
                alarm->time = time(NULL) + alarm->seconds;

                /*
                * Insert the new alarm into the list of alarms,
                * sorted by expiration time.
                */
                last = &alarm_list;
                next = *last;
                while (next != NULL) {//go through the linked list of alarms
                    if ((next->Alarm_ID > alarm->Alarm_ID) || (next->Alarm_ID == alarm->Alarm_ID && next->time >= alarm->time)) {//if the current alarm trigger time is greater than or equal to the new alarms' time, then this is a correct postion to insert. Ryan: also added the ID check
                        alarm->link = next;
                        *last = alarm;
                        break;
                    }
                    last = &next->link;
                    next = next->link;
                }
                /*
                * If we reached the end of the list, insert the new
                * alarm there. ("next" is NULL, and "last" points
                * to the link field of the last item, or to the
                * list header).
                */
                if (next == NULL) {//if we reach the end of the list
                    *last = alarm;
                    alarm->link = NULL;
                }
                printf("Alarm(%d) Inserted by Main Thread(%ld) Into Alarm List at %ld: %s %d %s\n", alarm->Alarm_ID, (unsigned long)pthread_self(), time(NULL), alarm->Type, alarm->seconds, alarm->message);//print the output
                print_alarm_list(); // print the list(just for debugging)

    #ifdef DEBUG//if we are in debug mode, then this will print the current state of the alarm list, with each alarm's trigger time and message
                printf ("[list: ");
                for (next = alarm_list; next != NULL; next = next->link)
                    printf ("%d(%d)[\"%s\"] ", next->time,
                        next->time - time (NULL), next->message);
                printf ("]\n");
    #endif
                status = pthread_mutex_unlock (&alarm_mutex);//after insert the new alarm into the list, unlocked mutex, allow to access this list again
                if (status != 0)
                    err_abort (status, "Unlock mutex");
            }
        }

        // Handle Change_Alarm case
        else if (strncmp(line, "Change_Alarm", 12) == 0) {
            int alarm_id;
            char new_type[10];
            int new_seconds;
            char new_message[128];

            if (sscanf(line, "Change_Alarm(%d): %s %d %128[^\n]", &alarm_id, new_type, &new_seconds, new_message) < 4) {
                fprintf(stderr, "Bad Change_Alarm command\n");
                free(alarm);
                continue;
            }

            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            if (alarm_list == NULL) {
            alarm_list = alarm;
            alarm->link = NULL;
        } else {
            last = &alarm_list;
            next = *last;
        }

            while (next != NULL) {
                if (next->Alarm_ID == alarm_id) {


                    // Update the existing alarm fields without changing the Alarm_ID
                    strncpy(next->Type, new_type, sizeof(next->Type) - 1);
                    next->Type[sizeof(next->Type) - 1] = '\0'; // Ensure null termination
                    next->seconds = new_seconds;
                    next->time = time(NULL) + new_seconds;
                    strncpy(next->message, new_message, sizeof(next->message) - 1); // Use strncpy to avoid overflow
                    next->message[sizeof(next->message) - 1] = '\0'; // Ensure null termination

                    // No need to reinsert the node, just update the current one
                    printf("Alarm(%d) Changed by Main Thread(%ld) at %ld: %s %d %s\n",
                        next->Alarm_ID, (unsigned long)pthread_self(), time(NULL),
                        next->Type, next->seconds, next->message);

                    break; // Exit the loop as the change is complete
                }

                last = &next->link;
                next = next->link;
            }

            // If the specified Alarm_ID was not found in the list, print an error message
            if (next == NULL) {
                printf("Alarm(%d) not found. Cannot change.\n", alarm_id);
            }

#ifdef DEBUG
            printf("[list: ");
            for (alarm_t *curr = alarm_list; curr != NULL; curr = curr->link) {
                printf("%d(%d)[\"%s\"] ", curr->time, curr->time - time(NULL), curr->message);
            }
            printf("]\n");
#endif

            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock mutex");
        }
        
        // Handle Cancel_Alarm case
        else if (strncmp(line, "Cancel_Alarm", 12) == 0) {
            int alarm_id;

            if (sscanf(line, "Cancel_Alarm(%d)", &alarm_id) == 1) {
                status = pthread_mutex_lock(&alarm_mutex);
                if (status != 0)
                    err_abort(status, "Lock mutex");

                last = &alarm_list;
                next = *last;
                while (next != NULL) {
                    if (next->Alarm_ID == alarm_id) {
                        printf("Alarm(%d) Cancelled by Main Thread(%ld) at %ld: %s %d %s\n",
                               next->Alarm_ID, (unsigned long)pthread_self(), time(NULL),
                               next->Type, next->seconds, next->message);
                        *last = next->link;
                        free(next);
                        break;
                    }
                    last = &next->link;
                    next = next->link;
                }

                if (next == NULL) {
                    printf("Alarm(%d) not found. Cannot cancel.\n", alarm_id);
                }

#ifdef DEBUG
                printf("[list: ");
                for (alarm_t *curr = alarm_list; curr != NULL; curr = curr->link) {
                    printf("%d(%d)[\"%s\"] ", curr->time, curr->time - time(NULL), curr->message);
                }
                printf("]\n");
#endif

                status = pthread_mutex_unlock(&alarm_mutex);
                if (status != 0)
                    err_abort(status, "Unlock mutex");
            } else {
                fprintf(stderr, "Bad Cancel_Alarm command\n");
            }
        }
        
        // Handle View_Alarms command
        else if (strncmp(line, "View_Alarms", 11) == 0) {
            if (strcmp(line, "View_Alarms\n") == 0) {
                status = pthread_mutex_lock(&alarm_mutex);
                if (status != 0)
                    err_abort(status, "Lock mutex");

                // Get the current time for the view command
                time_t view_time = time(NULL);
                printf("View Alarms at %ld:\n", view_time);

                // Check if there are any display threads
                if (display_thread_count == 0) {
                    printf("No display threads available\n");
                } else {
                    int display_thread_num = 1;  // Display thread numbering starts from 1

                    // Iterate over each display thread
                    for (int i = 0; i < display_thread_count; i++) {
                        display_thread_t *dt = display_threads[i];

                        // Print the display thread information
                        printf("%d. Display Thread %lu Assigned:\n", display_thread_num, (unsigned long)dt->thread_id);

                        // Iterate over alarms assigned to this display thread
                        char sub_label = 'a';
                        for (int j = 0; j < dt->alarm_count; j++) {
                            alarm_t *alarm = dt->alarms[j];
                            if (alarm != NULL) {
                                // Print each alarm with sub-label (1a, 1b, 2a, etc.)
                                printf(" %d%c. Alarm(%d): %s %d %s\n",
                                    display_thread_num, sub_label,
                                    alarm->Alarm_ID, alarm->Type, alarm->seconds, alarm->message);
                                sub_label++;
                            }
                        }

                        display_thread_num++;  // Increment for the next display thread
                    }
                }

                status = pthread_mutex_unlock(&alarm_mutex);
                if (status != 0)
                    err_abort(status, "Unlock mutex");

                printf("alarm> ");
                fflush(stdout);
            } else {
                printf("Bad View_Alarms command\n");
            }
        } else {
            fprintf(stderr, "Unrecognized command\n");
        }


    }
}
