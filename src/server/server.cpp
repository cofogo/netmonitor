#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "shared_data.h"

int main(int argc, char* argv[0])
{
    key_t shm_key;
    int shm_id;
    struct Sh_mem_data* shm_ptr;

    //if(argc != 5) {
    //    printf("Use: %s #1 #2 #3 #4\n", argv[0]);
    //    exit(1);
    //}

    shm_key = ftok(".", 'a');
    shm_id = shmget(shm_key, sizeof(struct Sh_mem_data), IPC_CREAT | 0666);
    if(shm_id < 0) {
        printf("*** ERROR: server failed to get shared memory ***\n");
        exit(1);
    }
    printf("Server received shared memory.\n");

    shm_ptr = (struct Sh_mem_data *) shmat(shm_id, NULL, 0);
    if((long int) shm_ptr == -1) {
        printf("*** ERROR: server failed to attach shared memory ***\n");
        exit(1);
    }
    printf("Server attached shared memory.\n");

    unsigned max_up = 180;
    for(unsigned i = 0; i < max_up; ++i) {
        printf("Listen uptime is %ds/%d.\n", i, max_up);
        printf("Reading shared memory.\n");
        //shm_ptr->status = NOT_READY;
        //shm_ptr->data[0] = atoi(argv[1]);
        //shm_ptr->data[1] = atoi(argv[2]);
        //shm_ptr->data[2] = atoi(argv[3]);
        //shm_ptr->data[3] = atoi(argv[4]);
        printf("Shared memory ethernet link data:\n%s\n", shm_ptr->e_link_data);
        printf("Shared memory ethernet addr data:\n%s\n", shm_ptr->e_addr_data);
        printf("Shared memory wlan link data:\n%s\n", shm_ptr->w_link_data);
        printf("Shared memory wlan addr data:\n%s\n", shm_ptr->w_addr_data);
        //printf("server filled shared memory with %d %d %d %d ...\n", 
        //        shm_ptr->data[0], shm_ptr->data[1],
        //        shm_ptr->data[2], shm_ptr->data[3]);
        //shm_ptr->status = FILLED;
        printf("Will sleep for a bit.\n\n");
        sleep(1);
    }

    //printf("Please start the client in another window...\n");

    //int max_wait = 20;
    //int wait_count = 0;
    //printf("Server will wait for %d seconds...\n", max_wait);
    //while(shm_ptr->status != TAKEN) {
    //    sleep(1);
    //    ++wait_count;
    //    if(wait_count == max_wait) {
    //        printf("Server timeout!...\n");
    //        break;
    //    }
    //    printf("Server waiting, time elapsed %d/%d...\n", wait_count, max_wait);
    //}

    //if(wait_count < max_wait) {
    //    printf("Server has detected the completion of client...\n");
    //}

    shmdt((void *) shm_ptr);
    printf("Server has detached its shared memory...\n");
    shmctl(shm_id, IPC_RMID, NULL);
    printf("Server has removed its shared memory...\n");

    printf("Server is exiting...\n");
    exit(0);
}
