#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct job_queue{
	int *buffer;
	int capacity;
	int size;
	int wp;
	int rp;
	pthread_mutex_t lock;
	pthread_cond_t writeable; 
	pthread_cond_t readable; 
} job_queue;

job_queue *init_ring_buffer(int capacity) {
	job_queue *rb = (job_queue*) malloc(sizeof(job_queue));;
	rb->capacity = capacity;
	rb->buffer = malloc(capacity * sizeof(int));
	rb->size = 0;
	rb->wp = 0;
	rb->rp = 0;
	pthread_mutex_init(&rb->lock, NULL);
	pthread_cond_init(&rb->writeable, NULL);
	pthread_cond_init(&rb->readable, NULL);
	return rb;
}

void free_ring_buffer(job_queue* rb) {
	pthread_cond_destroy(&rb->writeable);
	pthread_cond_destroy(&rb->readable);
	pthread_mutex_destroy(&rb->lock);
	free(rb->buffer);
	free(rb);
}


void add(int* data, job_queue* rb){
	pthread_mutex_lock(&rb->lock);
	while (rb->size == rb->capacity) {
	pthread_cond_wait(&rb->writeable, &rb->lock);
	}
	rb->buffer[rb->wp] = *data;
	rb->wp = (rb->wp +1) % rb->capacity;
	rb->size++;
	pthread_cond_signal(&rb->readable);
	pthread_mutex_unlock(&rb->lock);
}

int get(job_queue* rb){
	pthread_mutex_lock(&rb->lock);
	while (rb->size == 0) {
	pthread_cond_wait(&rb->readable, &rb->lock);
	}
	int data = rb->buffer[rb->rp];
	rb->rp = (rb->rp +1) % rb->capacity;
	rb->size--;
	pthread_cond_signal(&rb->writeable);
	pthread_mutex_unlock(&rb->lock);
	return data;
}

pid_t gettid(void);
void add_to_buffer(void* ringb){
	job_queue *rb = ringb;
	int i = 0;
	while (1) {
	printf("%2d -> [%d]       %d\n", i, rb->wp, gettid());
	add(&i, rb);
	i = (i+1)%100;
	}
}
void read_from_buffer(void* ringb){
	job_queue *rb = ringb;
	while (1) {
	int data = get(rb);
	printf("      [%d] -> %2d %d\n", rb->rp, data, gettid());
	}
}

int main() {
	int n = 6;
	int m = 6;
	pthread_t p[n], c[m];
	void *rb = init_ring_buffer(8);
	for (int i=0; i< n; i++) {
	pthread_create(&p[i], NULL, (void*) add_to_buffer, (void*) rb);
	}
	for (int i=0; i< m; i++) {
	pthread_create(&c[i], NULL, (void*) read_from_buffer, (void*) rb);
	}
	for (int i=0; i< n; i++) {
	pthread_join(p[i], NULL);
	}
	for (int i=0; i< m; i++) {
	pthread_join(c[i], NULL);
	}
	free_ring_buffer(rb);
}

