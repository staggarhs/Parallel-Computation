#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "load.h"

#define num_stages 4

int result;

typedef struct _stage_ {
    pthread_mutex_t     mutex;          /* Protect data */
    pthread_cond_t      avail;          /* Data available */
    pthread_cond_t      ready;          /* Ready for data */
    int                 data_ready;     /* Data present */
    long                data;           /* Data to process */
	struct _stage_      *next;          /* Next stage */
    pthread_t           thread;         /* Thread for stage */
	int					id;
} stage_t;


typedef struct _pipe_ {
    stage_t             *head;          /* First stage */
    int                 stages;         /* Number of stages */
} pipe_t;


void pipe_send (stage_t *stage, long data){

    pthread_mutex_lock (&stage->mutex);
    while (stage->data_ready) 
        pthread_cond_wait (&stage->ready, &stage->mutex);
	stage->data = data;
	stage->data_ready = 1;
    pthread_cond_signal (&stage->avail);
    pthread_mutex_unlock (&stage->mutex);
}


void *pipe_stage (void *arg){

    stage_t *stage = (stage_t*)arg;
    stage_t *next_stage = stage->next;
    pthread_mutex_lock (&stage->mutex);
    
	while (stage->data_ready != 1) {
		pthread_cond_wait (&stage->avail, &stage->mutex);
	}
	if ( stage->next == NULL){
		result = stage->data + 1;
	}
	else pipe_send (next_stage, stage->data + 1);
	
	stage->data_ready = 0;
	pthread_cond_signal (&stage->ready);
		
}


void pipe_create (pipe_t *pipe, int stages){

    int pipe_index;
	stage_t *new_stage, *stage;
    
    pipe->stages = stages;
    pipe->head = NULL;
	
	stage = (stage_t*)malloc(sizeof (stage_t));
	
	if (stage == NULL){
		printf("Error allocating memory\n");
		exit(1);
	}
	pipe->head = stage;
	
    for (pipe_index = 1; pipe_index <= stages; pipe_index++) {
        
		pthread_mutex_init (&stage->mutex, NULL);
        pthread_cond_init (&stage->avail, NULL);
        pthread_cond_init (&stage->ready, NULL);
        stage->data_ready = 0;
        
        new_stage = (stage_t*)malloc(sizeof (stage_t));
		
        if (new_stage == NULL){
			printf("Error allocating memory\n");
			exit(1);
		}
		
        if (pipe_index == stages){
			stage->next = NULL;
		}
        else { 
        	stage->next = new_stage;
        	stage = new_stage;
        }
        
    }

    for (stage = pipe->head; stage != NULL; stage = stage->next) {
        pthread_create (&stage->thread, NULL, pipe_stage, (void*)stage);
    }
	
}


void pipe_start (pipe_t *pipe, long value){
  
    pipe_send (pipe->head, value);
}


void pipe_destroy(pipe_t *pipe){
	stage_t *stage;
	
	for (stage = pipe->head; stage != NULL; stage = stage->next) {
		pthread_join (stage->thread, NULL);
		free(stage);		
	}
 }


int main (){

    pipe_t my_pipe;
	
	pipe_create (&my_pipe, 2);
    pipe_start (&my_pipe, num_stages);
	pipe_destroy (&my_pipe);
		
	printf ("Result is %d\n", result);
}

