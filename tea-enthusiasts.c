#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

/*
   CS 370 Project 5 - Tea Enthusiast Problem
   6 supplier threads (one per ingredient), as allowed by the project clarification.
 */

#define NUM_INGREDIENTS 6
#define NUM_SUPPLIERS   6
#define NUM_ENTHUSIASTS 14

// Ingredient indices
enum {
    GREEN_TEA = 0,
    BLACK_TEA,
    BERGAMOT_OIL,
    HIBISCUS_LEAF,
    PUFFED_RICE,
    SPICE
};

const char *ingredient_names[NUM_INGREDIENTS] = {
    "Green Tea Leaf",
    "Black Tea Leaf",
    "Bergamot Oil",
    "Hibiscus Leaf",
    "Puffed Rice",
    "Spice"
};

// Shared data
int pouches[NUM_INGREDIENTS];

// Synchronization primitives
pthread_mutex_t table_mutex;
sem_t supplier_sems[NUM_INGREDIENTS];
sem_t enthusiasts_done_sem;

// Arguments for threads
typedef struct {
    int id;
} supplier_arg_t;

typedef struct {
    int id;
} enthusiast_arg_t;

// Thread functions
void *supplier_thread(void *arg) {
    supplier_arg_t *info = (supplier_arg_t *)arg;
    int id = info->id;

    
    (void)id;

    pthread_exit(NULL);
}

void *enthusiast_thread(void *arg) {
    enthusiast_arg_t *info = (enthusiast_arg_t *)arg;
    int id = info->id;

    (void)id;

    pthread_exit(NULL);
}

int main(void) {
    printf("CS 370 Project 5 - Tea Enthusiast Problem - Solved by Seunghun Lee\n\n");

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Initialize shared pouches array to 0
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        pouches[i] = 0;
    }

    // Initialize mutex
    if (pthread_mutex_init(&table_mutex, NULL) != 0) {
        fprintf(stderr, "Error: pthread_mutex_init failed\n");
        return 1;
    }

    // Initialize semaphores for suppliers
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        if (sem_init(&supplier_sems[i], 0, 0) != 0) {
            fprintf(stderr, "Error: sem_init (supplier_sems[%d]) failed\n", i);
            return 1;
        }
    }

    // Semaphore to track enthusiasts finish
    if (sem_init(&enthusiasts_done_sem, 0, 0) != 0) {
        fprintf(stderr, "Error: sem_init (enthusiasts_done_sem) failed\n");
        return 1;
    }

    // Create supplier and enthusiast threads

    pthread_t supplier_threads[NUM_SUPPLIERS];
    pthread_t enthusiast_threads[NUM_ENTHUSIASTS];

    supplier_arg_t supplier_args[NUM_SUPPLIERS];
    enthusiast_arg_t enthusiast_args[NUM_ENTHUSIASTS];

    // Create supplier threads
    for (int i = 0; i < NUM_SUPPLIERS; i++) {
        supplier_args[i].id = i;  /* same as ingredient index */
        if (pthread_create(&supplier_threads[i], NULL, supplier_thread, &supplier_args[i]) != 0) {
            fprintf(stderr, "Error: pthread_create supplier %d failed\n", i);
            return 1;
        }
    }

    // Create enthusiast threads
    for (int i = 0; i < NUM_ENTHUSIASTS; i++) {
        enthusiast_args[i].id = i + 1;
        if (pthread_create(&enthusiast_threads[i], NULL, enthusiast_thread, &enthusiast_args[i]) != 0) {
            fprintf(stderr, "Error: pthread_create enthusiast %d failed\n", i + 1);
            return 1;
        }
    }

    // Join enthusiast threads
    for (int i = 0; i < NUM_ENTHUSIASTS; i++) {
        pthread_join(enthusiast_threads[i], NULL);
    }

    // Join supplier threads
    for (int i = 0; i < NUM_SUPPLIERS; i++) {
        pthread_join(supplier_threads[i], NULL);
    }

    // Destroy mutex and semaphores
    pthread_mutex_destroy(&table_mutex);
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        sem_destroy(&supplier_sems[i]);
    }
    sem_destroy(&enthusiasts_done_sem);

    printf("Program finished (threads joined, resources cleaned).\n");
    return 0;
}
