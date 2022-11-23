#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

static struct timeval start_time; // simulation start time

struct customer_info{
    int user_id;
    int class_type;
    int service_time;
    int arrival_time;
};
struct clerk_info{
    int id;
};

struct customer_info queue[2][30]; //queue[0] is the economy line and queue[1]is the buisness line

int item_count[2] = {0,0}; //item_count[0] is the length of the economy line and item_count[1] is the length of the buisness line


pthread_mutex_t mutex_class[2]; //one mutex for each queue
pthread_cond_t wave[2]; //one wave condition for each line to broadcast to customers from clerks
pthread_cond_t finish[4]; //one finish condition for each clerk to recieve as a signal from customers finishing service


int total_custs = 0;
int total_clerks = 4;
int custs_processed = 0; //total custs processed

int wave_id[2] = {-1,-1}; //identifys the clerk waving to line [0] and [1]. -1 if idle
int winner_selected [2] = {0,0}; //identifys when a customer has dequed from a line and is on their way to a clerk

int wait_times[3][30]; //wait_times[0] - wait times for queue 0. wait_times[1] - wait_times for queue 1. wait_times[2] - wait times for all queues
int wait_times_len[3] = {0,0,0}; //current length of wait time arrays

double getCurrentSimulationTime(){
	
	struct timeval cur_time;
	double cur_secs, init_secs;
	
	//pthread_mutex_lock(&start_time_mtex); you may need a lock here
	init_secs = (start_time.tv_sec + (double) start_time.tv_usec / 1000000);
	//pthread_mutex_unlock(&start_time_mtex);
	
	gettimeofday(&cur_time, NULL);
	cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
	
	return cur_secs - init_secs;
}

//data (customer) gets in line
void enqueue(struct customer_info data) {
    int class = data.class_type;
    queue[class][item_count[class]] = data; //add to respective queue
    item_count[class]++;
}
//dequeue from line class
struct customer_info dequeue(int class) {
   struct customer_info curr = queue[class][0];
   for(int i = 1;i < item_count[class];i++){ //repopulate array
      queue[class][i-1] = queue[class][i];
   }
   item_count[class]--;
   return curr;
}


void* clerk_entry(void* clerk_info){
    struct clerk_info* clerk = (struct info_node *) clerk_info;
    while(1){
        if(custs_processed >= total_custs){ //end if all customers from file have been served
            pthread_exit(NULL);
            return NULL;
        }
        //check for available customers in line [1]
        if(item_count[1]!=0){
            pthread_mutex_lock(&mutex_class[1]);
            {
                winner_selected[1] = 1;
                pthread_cond_broadcast(&wave[1]); //wave to line 1
                wave_id[1] = clerk->id;
                if(pthread_cond_wait(&finish[clerk->id],&mutex_class[1])!=0) //wait for customer to finish being served
                    break;
                pthread_mutex_unlock(&mutex_class[1]);
            }
        //check for available customers in line [0]
        }else if(item_count[0]!=0){
            pthread_mutex_lock(&mutex_class[0]);
            {
                winner_selected[0] = 1;
                pthread_cond_broadcast(&wave[0]); //wave to line 2
                wave_id[0] = clerk->id;
                if(pthread_cond_wait(&finish[clerk->id],&mutex_class[0])) //wait for customer to finish being served
                    break;
                pthread_mutex_unlock(&mutex_class[0]);
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}

void* customer_entry(void* cust_info){
	struct customer_info * cust = (struct info_node *) cust_info;
    int class = cust->class_type; //line type
    int id = cust->user_id;
    double start_time = 0; //cust enters line
    double end_time = 0; //cust leaves line
    double goodbye_time = 0; //cust finishes being served

    

    usleep(cust->arrival_time*100000); //wait for arrival time in 10ths of a second
	fprintf(stdout, "A customer arrives: customer ID %2d. \n", id);

    pthread_mutex_lock(&mutex_class[class]);
    {
        enqueue(*cust);
        start_time = getCurrentSimulationTime();
        fprintf(stdout, "A customer with ID %d enters queue %d at %.2f. Length of the queue is now %d. \n", id, class, start_time, item_count[class]);

        while(1){
            pthread_cond_wait(&wave[class], &mutex_class[class]); //wait for wave from clerk
            if(queue[class][0].user_id==cust->user_id && winner_selected[class]){ //if next in line
                winner_selected[class] = 0;
                dequeue(class);
                fprintf(stdout, "Customer %d has dequeued from queue %d. Length of the queue is now %d\n", id, class, item_count[class]);
                break;
            }
        }
    }
    int clerk_id = wave_id[class];
    //calculate wait times
    end_time = getCurrentSimulationTime();
    wait_times[2][wait_times_len[2]] = end_time - start_time;
    wait_times_len[2]++;
    if(class == 0){
        wait_times[0][wait_times_len[0]] = end_time - start_time;
        wait_times_len[0]++;
    }else{
        wait_times[1][wait_times_len[1]] = end_time - start_time;
        wait_times_len[1]++;
    }
    pthread_mutex_unlock(&mutex_class[class]);
    fprintf(stdout,"Clerk %d starts serving customer %d at time %.2f\n", clerk_id, id, end_time);
    usleep(cust->service_time*100000); //wait for service time in 10ths of a second
    goodbye_time = getCurrentSimulationTime();
    fprintf(stdout, "Clerk %d finishes serving customer %d at time %.2f\n", clerk_id, id, goodbye_time);
    pthread_cond_signal(&finish[clerk_id]); //signal to clerk that customer is done

    custs_processed++;

    if(custs_processed >= total_custs){//if all custs processed
        double avg[3] = {0,0,0};

        for(int i = 0;i < wait_times_len[2];i++){
            avg[2]+=wait_times[2][i];
        }
        avg[2] /= (double)wait_times_len[2];
        fprintf(stdout, "The average waiting time for all customers in the system is %.2f seconds\n",avg[2]);

        for(int i = 0;i < wait_times_len[1];i++){
            avg[1]+=wait_times[1][i];
        }
        avg[1] /= (double)wait_times_len[1];
        fprintf(stdout, "The average waiting time for customers in buisness class is %.2f seconds\n",avg[1]);

        for(int i = 0;i < wait_times_len[0];i++){
            avg[0]+=wait_times[0][i];
        }
        avg[0] /= (double)wait_times_len[0];
        fprintf(stdout, "The average waiting time for customers in economy class is %.2f seconds\n",avg[0]);

        exit(0);
    }
    pthread_exit(NULL);   
    return NULL;
}

int main(int argc, char *argv[]){
    FILE *text_file;
    char line[1000];
    char *cust_info_format = "%d:%d,%d,%d";

	gettimeofday(&start_time, NULL); // record simulation start time

    pthread_mutex_init(&mutex_class[0], NULL);
    pthread_mutex_init(&mutex_class[1], NULL);
    pthread_cond_init(&wave[0],NULL);
    pthread_cond_init(&wave[1],NULL);
    pthread_cond_init(&finish[0],NULL);
    pthread_cond_init(&finish[1],NULL);
    pthread_cond_init(&finish[2],NULL);
    pthread_cond_init(&finish[3],NULL);

    if(argc!=2){
        printf("please supply one argument\n");
    }


    text_file = fopen(argv[1], "r");
    if(text_file==NULL){
        printf("file does not exist\n");
        return 1;
    }

    //get number of customers
    fgets(line, 1000, text_file);
    total_custs = atoi(line);

    pthread_t clerk_threads[total_clerks];
    struct clerk_info clerks[total_clerks];
    pthread_t cust_threads[total_custs];    
    struct customer_info customers[total_custs];

    //populate clerk structs
    for(int i = 0;i < total_clerks;i++){
        clerks[i].id = i;
    }
    //create clerk threads
    for(int i = 0;i < total_clerks; i++){
        pthread_create(&clerk_threads[i], NULL, &clerk_entry, (void*)&clerks[i]);
        usleep(10);
    }

    //create customer threads
    for(int i = 0;i < total_custs; i++){
        fgets(line, 1000, text_file);
        //populate list of customer info
        sscanf(line, cust_info_format, &customers[i].user_id, &customers[i].class_type, &customers[i].arrival_time, &customers[i].service_time);
        pthread_create(&cust_threads[i], NULL, &customer_entry, (void*)&customers[i]);
    }


    //join threads
    for(int i = 0;i < total_custs; i++){
        pthread_join(cust_threads[i], NULL);
    }
    for(int i = 0;i < total_clerks; i++){
        pthread_join(clerk_threads[i], NULL);
    }


    pthread_mutex_destroy(&mutex_class[0]);
    pthread_mutex_destroy(&mutex_class[1]);
    pthread_cond_destroy(&wave[0]);
    pthread_cond_destroy(&wave[1]);
    pthread_cond_destroy(&finish[0]);
    pthread_cond_destroy(&finish[1]);
    pthread_cond_destroy(&finish[2]);
    pthread_cond_destroy(&finish[3]);
    fclose(text_file);
    return 0;



}