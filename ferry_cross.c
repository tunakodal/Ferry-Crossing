#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>

#define FERRY_CAPACITY 5 // number of cars the ferry can hold
#define SIMULATION_TIME 60 // seconds

sem_t sem_board; // ticket
sem_t sem_full; // ready to go
sem_t sem_unboard; // safe to exit light
sem_t sem_empty; // clean sweep

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex for critical sections

int cars_on_board = 0; 
bool ferry_available = true; // when ferry is at the dock (not crossing or not unboarding)
bool simulation_running = true;
int next_car_id = 1; // to assign unique IDs to cars

struct timeval start_time;

double get_elapsed_time() {
    struct timeval now;
    int temp = gettimeofday(&now, NULL);
    if (temp != 0)
    {
        perror("gettimeofday");
        exit(EXIT_FAILURE);
    }
    
    return (now.tv_sec - start_time.tv_sec) + (now.tv_usec - start_time.tv_usec) / 1000000.0;
}

void log_event(const char *fmt, ...) {
    double elapsed = get_elapsed_time();

    printf("[Clock : %.2f] ", elapsed);
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void* car_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    // Wait for ferry to be available i.e. get ticket
    if (sem_wait(&sem_board) != 0)
    {
        perror("sem_wait(sem_board)");
        pthread_exit(NULL);
    }

    // Critical section to update cars_on_board + signal the ferry if full (if needed)
    if (pthread_mutex_lock(&mutex) != 0)
    {
        perror("pthread_mutex_lock");
        pthread_exit(NULL);
    }
    log_event("Car %d entered the ferry", id);
    cars_on_board++;

    if (cars_on_board == FERRY_CAPACITY) {
        if (sem_post(&sem_full) != 0)
        {
            perror("sem_post(sem_full)");
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
    }
    
    if (pthread_mutex_unlock(&mutex) != 0)
    {
        perror("pthread_mutex_unlock");
        pthread_exit(NULL);
    }

    if (sem_wait(&sem_unboard) != 0)
    {
        perror("sem_wait(sem_unboard)");
        pthread_exit(NULL);
    }


    // Critical section to update cars_on_board + signal the ferry if empty (if needed)
    if (pthread_mutex_lock(&mutex) != 0)
    {
        perror("pthread_mutex_lock");
        pthread_exit(NULL);
    }
    log_event("Car %d exited the ferry", id);
    cars_on_board--;

    if (cars_on_board == 0) {
        if (sem_post(&sem_empty) != 0)
        {
            perror("sem_post(sem_empty)");
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
    }

    if (pthread_mutex_unlock(&mutex) != 0)
    {
        perror("pthread_mutex_unlock");
        pthread_exit(NULL);
    }

    pthread_exit(NULL);

}

void* car_generator_thread(void *arg) {
    while (simulation_running)
    {
        if(ferry_available){
        pthread_t tid;

        // Allocating memory for car IDs
        int *car_id = malloc(sizeof(int));
        if (car_id == NULL)
        {
            perror("malloc");
            continue;
        }
        *car_id = next_car_id++;

        // Create car thread
        int car = pthread_create(&tid, NULL, car_thread, car_id);
        if (car != 0)
        {
            perror("pthread_create");
            continue;
        }

        // Random arrival time between 0.1 to 1 seconds
        usleep((rand() % 1000) * 1000);

        car = pthread_detach(tid);
        if (car != 0)
        {
            perror("pthread_detach");
            continue;
        }
        }   
    }
    pthread_exit(NULL);
}

void* ferry_thread(void *arg) {
    while (simulation_running)
    {
        double elapsed = get_elapsed_time();
        if (elapsed >= SIMULATION_TIME)
        {
            simulation_running = false;
            break;
        }


        ferry_available = true;
        // Allow cars to board (give tickets)
        for (int i = 0; i < FERRY_CAPACITY; i++) {
            if (sem_post(&sem_board) != 0)
            {
                perror("sem_post(sem_board)");
                pthread_exit(NULL);
            }
        }

        // Wait until ferry is full
        if (sem_wait(&sem_full) != 0)
        {
            perror("sem_wait(sem_full)");
            pthread_exit(NULL);
        }

        log_event("Ferry leaves the dock");
        ferry_available = false;

        sleep(3); // Simulate crossing time

        log_event("Ferry arrives to new dock");

        // Allow cars to unboard
        for (int i = 0; i < FERRY_CAPACITY; i++) {
            if (sem_post(&sem_unboard) != 0)
            {
                perror("sem_post(sem_unboard)");
                pthread_exit(NULL);
            }
        }

        // Wait until ferry is empty
        if (sem_wait(&sem_empty) != 0)
        {
            perror("sem_wait(sem_empty)");
            pthread_exit(NULL);
        }
        
        // No "ferry_available" before checking the simulation time again 
    }
    pthread_exit(NULL);
}

int main(void){
    if (gettimeofday(&start_time,NULL) != 0)
    {
        perror("gettimeofday");
        exit(EXIT_FAILURE);
    }

    // Random seed
    srand((unsigned int)(start_time.tv_sec ^ start_time.tv_usec));

    // Initialize semaphores
    if (sem_init(&sem_board, 0, 0) != 0)
    {
        perror("sem_init(sem_board)");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&sem_full, 0, 0) != 0)
    {
        perror("sem_init(sem_full)");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&sem_unboard, 0, 0) != 0)
    {
        perror("sem_init(sem_unboard)");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&sem_empty, 0, 0) != 0)
    {
        perror("sem_init(sem_empty)");
        exit(EXIT_FAILURE);
    }

    pthread_t ferry_tid, car_gen_tid;

    // Start ferry thread
    if (pthread_create(&ferry_tid, NULL, ferry_thread, NULL) != 0)
    {
        perror("pthread_create(ferry_thread)");
        exit(EXIT_FAILURE);
    }   

    // Start car generator thread
    if (pthread_create(&car_gen_tid, NULL, car_generator_thread, NULL) != 0)
    {
        perror("pthread_create(car_generator_thread)");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(ferry_tid, NULL) != 0)
    {
        perror("pthread_join(ferry_thread)");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_join(car_gen_tid, NULL) != 0)
    {
        perror("pthread_join(car_generator_thread)");
        exit(EXIT_FAILURE);
    }
    
    // Destroy semaphores
    sem_destroy(&sem_board);
    sem_destroy(&sem_full); 
    sem_destroy(&sem_unboard);
    sem_destroy(&sem_empty);

    // Destroy mutex
    pthread_mutex_destroy(&mutex);

    exit(EXIT_SUCCESS);
}

