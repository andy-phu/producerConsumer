#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 10
#define MAX_THREADS 16

pthread_cond_t notempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t notfull = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int itemsProduced = 0; //to keep track of how many items the multipe producers produced 
int currentItemCount = 0; //how many items are actually in the buffer 
int totalConsumed = 0; //keep track of how many were consumed

struct ProducerAttributes {
    int id;
    int* buffer;
    int bufferSize;
    int itemLimit;
    int currCount; //how many items producer produces
    int delay;
};

struct ConsumerAttributes{
    int id;
    int* buffer;
    int bufferSize;
    int itemLimit;  //the i requested by user 
    int currCount; //how many items consumer consumed
    int pAmount;   //the p requested by the user
    int cAmount; //the c requested by user
    int delay;
};

void *producer(void *arg) {
    struct ProducerAttributes *producerElement = (struct ProducerAttributes *)arg;
    int producerID = producerElement->id;
    int b = producerElement->bufferSize;
    int itemLimit = producerElement->itemLimit;
    //while the buffer is full continue waiting til the notFull signal is given from the consumer 
    while (1) {
        if (producerElement->delay == 1){
            usleep(500000);
        } 

        pthread_mutex_lock(&mutex);
        while (currentItemCount == b) {
            pthread_cond_wait(&notfull, &mutex);
        }
        //makes sure that the producer doesn't produce another item if it has reached its produce limit
        if (producerElement->currCount == producerElement->itemLimit){
            //printf("exiting out of producer loop\n");
            pthread_mutex_unlock(&mutex);
            break;
        }
        //iterate throughout to find an empty spot
        for(int x = 0; x < b;x++){
            if (producerElement->buffer[x] == -1){
                producerElement->buffer[x] = itemsProduced;

                //prints the statement before incrementing currentItemCount
                printf("producer_%d produced item %d\n", producerID, itemsProduced);
                //Produces an item and adds it to the buffer
                (producerElement->currCount)++;
                itemsProduced++;
                currentItemCount++;
                pthread_cond_signal(&notempty);
                // Break the loop after placing the item in the buffer
                break; 
            }
        }

        pthread_mutex_unlock(&mutex);

    }
    
    return NULL;
}

void *consumer(void *arg) {
    struct ConsumerAttributes *consumerElement = (struct ConsumerAttributes *)arg;
    int b = consumerElement->bufferSize;
    int p = consumerElement->pAmount;
    int c = consumerElement->cAmount;
    int i = consumerElement->itemLimit;
    int consumerID = consumerElement->id;
    int flag = 0;
    while (1) {
        //printf("going inside the consumer function\n");
        if(consumerElement->delay == 0 && flag == 0){
            usleep(500000);
        } 

        pthread_mutex_lock(&mutex);
        //while there isn't anything in the buffer wait while on the notEmpty condition
        while (currentItemCount == 0) {
            pthread_cond_wait(&notempty, &mutex);
        }

        //printf("curr count[%d]: %d\n equation: %d\n",consumerID, (consumerElement->currCount), ((p*i)/c));
        //checks if the amount of items consumer consued is less than or equal to (p*i)/c

        //iterate throughout to find a filled spot
        for(int x = 0; x < b;x++){
            if (consumerElement->buffer[x] == totalConsumed){
                //prints the statement before decrementing currentItemCount
                printf("consumer_%d consumed item %d\n", consumerID, consumerElement->buffer[x]);
                //initialize the consumed spot back to -1
                for (int y = x; y < b - 1; y++) {
                    consumerElement->buffer[y] = consumerElement->buffer[y + 1];
                }
                consumerElement->buffer[b - 1] = -1;
                // if (consumerElement->buffer[x] == (p*i)){
                //     printf("HERE\n");
                //     flag = 1;
                // }
                totalConsumed++;
                //decrement the current item count
                currentItemCount--;
                //incremment the curr count to show that a consumer thread consumed another item
                (consumerElement->currCount)++;
                //Signal notFull after a consumption
                break;
            }
        }
        if ((consumerElement->currCount) > ((p*i)/c)-1){
            pthread_mutex_unlock(&mutex);
            //printf("here %d , %d\n", consumerElement->currCount, (p*i)/c);
            break;
        }
        // printf("flag: %d\n", flag);
        // if (flag == 1){
        //     break;
        // }
        pthread_cond_signal(&notfull);
        pthread_mutex_unlock(&mutex);

    }
    //printf("exit\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    // Grabs the 5 numbers that represent [p c i b d]
    int p = atoi(argv[1]);
    int c = atoi(argv[2]);
    int i = atoi(argv[3]);
    int b = atoi(argv[4]);
    int d = atoi(argv[5]);

    if (c > (p * i)){
        printf("p: %d c: %d i:%d\n", p,c,i);
        perror("c should be less than (p * i)");
        exit(1);
    }
    else if (b > MAX_BUFFER_SIZE){
        perror("buffer has to be less than 10");
        exit(1);
    }
    else if(p > 16 || c > 16){
        perror("the max thread limit for producers or consumers is 16");
        exit(1);
    }

    //initialize buffer if the element is -1 then it's not filled 
    int *buffer = malloc(b * sizeof(int));
    for(int x = 0; x < b;x++){
        buffer[x] = -1; 
    }
    pthread_t producer_threads[p];
    pthread_t consumer_threads[c];
    struct ProducerAttributes *producerArray = malloc(p * sizeof(struct ProducerAttributes));
    struct ConsumerAttributes *consumerArray = malloc(c * sizeof(struct ConsumerAttributes));

    //initialize the producer attributes
    for (int x = 0; x < p; ++x) {
        producerArray[x].id = x;
        producerArray[x].buffer = buffer; // Assign the buffer to each producer so that both functions can use it 
        producerArray[x].bufferSize = b;
        producerArray[x].itemLimit = i;
        producerArray[x].currCount = 0;
        producerArray[x].delay = d;
    }

    for (int x = 0; x < c; ++x) {
        consumerArray[x].id = x;
        consumerArray[x].buffer = buffer; // Assign the buffer to each producer so that both functions can use it 
        consumerArray[x].bufferSize = b;
        consumerArray[x].itemLimit = i;
        consumerArray[x].currCount = 0;
        consumerArray[x].pAmount = p;
        consumerArray[x].cAmount = c;
        consumerArray[x].delay = d;
    }


    int delay = 500000; 

    for (int x = 0; x < p; x++) {
        //printf("creating producer threads\n");
        // Create producer threads
        pthread_create(&producer_threads[x], NULL, producer, (void *)&producerArray[x]);
        // if (d == 1){
        //     usleep(delay);
        // }
    }

    for (int x = 0; x < c; x++) {
        // Create consumer threads
        pthread_create(&consumer_threads[x], NULL, consumer, (void *)&consumerArray[x]);
        // if (d == 0){
        //     usleep(delay);
        // }
    }

    for (int x = 0; x < p; x++) {
        // Join producer threads
        pthread_join(producer_threads[x], NULL);
    }

    for (int x = 0; x < c; x++) {
        // Join consumer threads
        pthread_join(consumer_threads[x], NULL);
    }

    free(buffer);
    free(producerArray);
    free(consumerArray);

    return 0;
}
