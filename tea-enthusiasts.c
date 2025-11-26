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
pthread_mutex_t table_mutex; // protects access to pouches[]
sem_t supplier_sems[NUM_INGREDIENTS];  // use to wake up suppliers
int stop_suppliers = 0; // when set to 1, suppliers will finish

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

        while (1) {
        // Wait until someone wakes me up
        sem_wait(&supplier_sems[id]);

        // Check if we are asked to stop
        if (stop_suppliers) {
            break;
        }

        // Generate a random number of pouches to add: 1~10
        int amount = (rand() % 10) + 1;

        // Update the shared table under mutex protection
        pthread_mutex_lock(&table_mutex);

        pouches[id] += amount;

        printf("\033[0;91m%s supplier added %d pouches of %s to the table.\n\033[0m",
               ingredient_names[id], amount, ingredient_names[id]);

        fflush(stdout);

        pthread_mutex_unlock(&table_mutex);
    }

    // When loop exits, supplier is finishing
    printf("\033[0;91m%s supplier is done\n\033[0m", ingredient_names[id]);
    fflush(stdout);

    pthread_exit(NULL);
}

void *enthusiast_thread(void *arg) {
    enthusiast_arg_t *info = (enthusiast_arg_t *)arg;
    int id = info->id;

    // Each enthusiast tries between 15 and 25 recipes
    int num_recipes = (rand() % 11) + 15;   // 15~25

    for (int r = 0; r < num_recipes; r++) {

        // step 1: Generate how many pouches are needed per ingredients
        int needed[NUM_INGREDIENTS] = {0};

        // Tea leaves: 0 or 1 each, but at least one leaf overall
        int use_green = rand() % 2;   // 0 or 1
        int use_black = rand() % 2;   // 0 or 1

        if (use_green == 0 && use_black == 0) {
            // if both 0 -> make one of them to 1
            if (rand() % 2 == 0) use_green = 1;
            else use_black = 1;
        }

        needed[GREEN_TEA] = use_green;
        needed[BLACK_TEA] = use_black;

        // choose 1~4 of the remaining 4 materials randomly and set them up to need 1~4 materials
        int extra_kinds = (rand() % 4) + 1;   // 1~4
        int chosen = 0;
        while (chosen < extra_kinds) {
            int ing = (rand() % 4) + 2;       // 2~5
            if (needed[ing] == 0) {
                needed[ing] = (rand() % 4) + 1;  // 1~4 pouch
                chosen++;
            }
        }

        // step 2: output recipe request content
        printf("\033[0;92mTea enthusiast %d is requesting tea with these many pouches:\n", id);
        for (int i = 0; i < NUM_INGREDIENTS; i++) {
            printf("  %d %s\n", needed[i], ingredient_names[i]);
        }
        printf("\033[0m");
        fflush(stdout);

        // step 3
        int ready = 0;

        while (!ready) {
            ready = 1;

            pthread_mutex_lock(&table_mutex);

            // check that all materials are sufficient
            for (int i = 0; i < NUM_INGREDIENTS; i++) {
                if (pouches[i] < needed[i]) {
                    // If there is any missing material, call supplier
                    if (needed[i] > 0) {
                        sem_post(&supplier_sems[i]);
                    }
                    ready = 0;
                }
            }

            // If all materials are sufficient, subtract them from the pouches and prepare the finished output
            if (ready) {
                for (int i = 0; i < NUM_INGREDIENTS; i++) {
                    pouches[i] -= needed[i];
                }

                printf("\033[0;92mTea for enthusiast %d is prepared\n\033[0m", id);
                fflush(stdout);

                pthread_mutex_unlock(&table_mutex);
                break;
            }

            pthread_mutex_unlock(&table_mutex);

            // provide some time for supplier
            usleep(2000);
        }

        // A little bit of a term between recipes
        usleep(2000);
    }

    printf("\033[0;92mTea enthusiast %d is done\n\033[0m", id);
    fflush(stdout);

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

    // Create supplier and enthusiast threads

    pthread_t supplier_threads[NUM_SUPPLIERS];
    pthread_t enthusiast_threads[NUM_ENTHUSIASTS];

    supplier_arg_t supplier_args[NUM_SUPPLIERS];
    enthusiast_arg_t enthusiast_args[NUM_ENTHUSIASTS];

    // Create supplier threads - one per ingredient
    for (int i = 0; i < NUM_SUPPLIERS; i++) {
        supplier_args[i].id = i;  // same as ingredient index
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

    printf("Tea enthusiasts finished running\n");

    // Tell suppliers to stop and wake them up
    stop_suppliers = 1;
    for (int i = 0; i < NUM_SUPPLIERS; i++) {
        sem_post(&supplier_sems[i]);
    }

    // Join supplier threads
    for (int i = 0; i < NUM_SUPPLIERS; i++) {
        pthread_join(supplier_threads[i], NULL);
    }

    printf("Suppliers finished running\n");

    // Destroy mutex and semaphores
    pthread_mutex_destroy(&table_mutex);
    for (int i = 0; i < NUM_INGREDIENTS; i++) {
        sem_destroy(&supplier_sems[i]);
    }

    return 0;
}
