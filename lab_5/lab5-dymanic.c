#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include "omp.h"

#define A 700 // Рачковский Дмитрий Эдуардович
#define NUMBER_OF_ITERATIONS 50
#define NUMBER_OF_THREADS 6

typedef enum stage_ {
    STAGE_UNDEFINED = 0,
    STAGE_GENERATE = 0,
    STAGE_MAP,
    STAGE_MERGE,
    STAGE_SORT,
    STAGE_REDUCE,
    STAGE_NUM_OF,
} stage_t;

typedef struct arg_struct_ {
    long start;
    long finish;
} arg_struct_t;

pthread_t comp_threads[NUMBER_OF_THREADS];
pthread_t progress_thread;
static pthread_mutex_t reduce_mutex = PTHREAD_MUTEX_INITIALIZER;

long N;
long double *M1;
long double *M2;
long double *M2_copy;

long check;

long double min;
double X = 0;
int map_done = 0;

long current_iteration = 0;
stage_t current_stage = STAGE_UNDEFINED;

double get_time_ms() { struct timeval T; gettimeofday(&T, NULL); return T.tv_sec + T.tv_usec / 1000000.; }

void *monitor_execution_percent() {
    while (1) {
        sleep(1);
        double execution_percent = (100. / NUMBER_OF_ITERATIONS) * current_iteration + (100. / NUMBER_OF_ITERATIONS) * ((double)current_stage / STAGE_NUM_OF);
        if (execution_percent < 100) {
            printf("Current progress: %.2f%%\n", execution_percent);
        } else {
            break;
        }
    }
    pthread_exit(NULL);
}

long double random_on_interval(long double min, long double max, unsigned int *seed) {
    return (long double) ((rand_r(seed) % (int)(max + 1 - min)) + min);
}

unsigned int make_seed(long i, long j) {
    return 9572 + 234*i + 456*j;
}

void start_threads(void * (*func)(void *), long *segments) {
    for (int j = 0; j < NUMBER_OF_THREADS; j++) {
        arg_struct_t *args = malloc(sizeof(arg_struct_t));
        args->start = segments[j];
        args->finish = segments[j+1];
        pthread_create(&comp_threads[j], NULL, func, (void *)args);
    }
}

void wait_for_threads() {
    for (int j = 0; j < NUMBER_OF_THREADS; j++) {
        pthread_join(comp_threads[j], NULL);
    }
}

void *fill_M1(void *arguments) {
    arg_struct_t *args = arguments;
    for (long j = args->start; j < args->finish; j++) {
        unsigned int seed = make_seed(current_iteration, j);
        M1[j] = random_on_interval(0, A, &seed);
    }
    pthread_exit(NULL);
}

void *fill_M2(void *arguments) {
    arg_struct_t *args = arguments;
    for (long j = args->start; j < args->finish; j++) {
        unsigned int seed = make_seed(current_iteration, j);
        M2[j] = random_on_interval(A, A*10, &seed);
    }
    pthread_exit(NULL);
}

void *map_M1(void *arguments) {
    arg_struct_t *args = arguments;
    for (long j = args->start; j < args->finish; j++) {
        // operation #1, remember to convert to radians
        M1[j] = pow(sinhl((M1[j] * M_PI) / 180.0), 2);
    }
    pthread_exit(NULL);
}

void *map_M2(void *arguments) {
    arg_struct_t *args = arguments;
    for (long j = args->start; j < args->finish; j++) {
        // operation #3
        M2[j] = fabs(tanl(M2[j] + M2_copy[j]));
    }
    pthread_exit(NULL);
}

void *merge(void *thread_num_id) {
    long *thread_num = (long *) thread_num_id;
    while (1) {
        if ((check >= N/2) || (*thread_num >= N/2)) {
            pthread_exit(NULL);
        } else if (*thread_num >= 0) {
            check++;
            M2[*thread_num] = pow(M1[*thread_num], M2[*thread_num]);
            *thread_num = -1;
        }
    }
}

void *sort(void *arguments) {
    arg_struct_t *args = arguments;
    for (long j = args->start; j < args->finish; j++) {
        int min_j = j;
        for (int k = j+1; k < args->finish; k++)  {
            if (M2[k] < M2[min_j]) {
                min_j = k;
            }
        }
        if (min_j != j) {
            long double temp = M2[j];
            M2[j] = M2[min_j];
            M2[min_j] = temp;
        }
    }
    pthread_exit(NULL);
}

void *reduce(void *arguments) {
    arg_struct_t *args = arguments;
    for (long j = args->start; j < args->finish; j++) {
        if (isfinite(M2[j]) && (int)(M2[j] / min) % 2 == 0) {
            // remember to convert to radians
            double value_to_add = sinl((M2[j] * M_PI) / 180.0);
            pthread_mutex_lock(&reduce_mutex);
            X += value_to_add;
            pthread_mutex_unlock(&reduce_mutex);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    double T1, T2, last_time_measure;
    double gen = 0;
    double map = 0;
    double mer = 0;
    double sor = 0;
    double red = 0;
    N = atoi(argv[1]);

    M1 = malloc(sizeof(long double) * N);
    M2 = malloc(sizeof(long double) * (N/2));
    M2_copy = malloc(sizeof(long double) * (N/2));

    long M1_segments[NUMBER_OF_THREADS + 1];
    long M2_segments[NUMBER_OF_THREADS + 1];
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        M1_segments[i] = ((N / NUMBER_OF_THREADS) + 1)  * i;
        M2_segments[i] = (((N/2) / NUMBER_OF_THREADS) + 1)  * i;
    }
    M1_segments[NUMBER_OF_THREADS] = N;
    M2_segments[NUMBER_OF_THREADS] = N/2;

    int j = 0;
    pthread_create(&progress_thread, NULL, monitor_execution_percent, NULL);
    T1 = get_time_ms();
    last_time_measure = T1;
    for (current_iteration=0; current_iteration<NUMBER_OF_ITERATIONS; current_iteration++) {
        /******************** GENERATE ********************/
        current_stage = STAGE_GENERATE;

        start_threads(fill_M1, M1_segments);
        wait_for_threads();

        start_threads(fill_M2, M2_segments);
        wait_for_threads();

        // make a copy, shifting M2 one element to the right
        M2_copy[0] = 0;
        memcpy(&M2_copy[1], M2, sizeof(long double) * ((N/2)-1));

        gen += get_time_ms() - last_time_measure;
        last_time_measure = get_time_ms();
        /******************** MAP *************************/
        current_stage = STAGE_MAP;

        start_threads(map_M1, M1_segments);
        wait_for_threads();

        start_threads(map_M2, M2_segments);
        wait_for_threads();

        map += get_time_ms() - last_time_measure;
        last_time_measure = get_time_ms();
        /******************** MERGE ***********************/
        current_stage = STAGE_MERGE;

        long *thread_indexes = malloc(sizeof(long) * NUMBER_OF_THREADS);
        for (j = 0; j < NUMBER_OF_THREADS; j++) {
            thread_indexes[j] = j;
        }
        long next_ind = 6;
        check = 0;
        for (j = 0; j < NUMBER_OF_THREADS; j++) {
            pthread_create(&comp_threads[j], NULL, merge, (void *)&thread_indexes[j]);
        }
        while (next_ind < N/2) {
            for (j = 0; j < NUMBER_OF_THREADS; j++) {
                if (thread_indexes[j] == -1) {
                    thread_indexes[j] = next_ind++;
                }
            }
        }
        wait_for_threads();

        mer += get_time_ms() - last_time_measure;
        last_time_measure = get_time_ms();
        /******************** SORT ************************/
        current_stage = STAGE_SORT;

        start_threads(sort, M2_segments);
        wait_for_threads();

        // we'll be reusing this variable
        memcpy(M2_copy, M2, sizeof(long double) * (N/2));
        long start_locations[NUMBER_OF_THREADS];
        memcpy(start_locations, M2_segments, sizeof(long) * NUMBER_OF_THREADS);
        for (j = 0; j < N/2; j++) {
            int min_interval = 0;
            for (int m = 1; m < NUMBER_OF_THREADS; m++) {
                if ((start_locations[m] < N/2) && (M2_copy[start_locations[m]] < M2_copy[start_locations[min_interval]])) {
                    min_interval = m;
                }
            }
            M2[j] = M2_copy[start_locations[min_interval]];
            M2_copy[start_locations[min_interval]] = INFINITY;
            start_locations[min_interval]++;
        }

        sor += get_time_ms() - last_time_measure;
        last_time_measure = get_time_ms();
        /******************** REDUCE **********************/
        current_stage = STAGE_REDUCE;
        int min_index;
        for (min_index = 0; M2[min_index] <= 0; min_index++) {}
        min = M2[min_index];

        start_threads(reduce, M2_segments);
        wait_for_threads();

        red += get_time_ms() - last_time_measure;
        last_time_measure = get_time_ms();
        current_stage = STAGE_UNDEFINED;
    }
    T2 = get_time_ms();


    long delta_ms = (T2 - T1) * 1000;
    printf("%ld\n%ld\n%ld\n%ld\n%ld\n", (long)(gen*1000), (long)(map*1000), (long)(mer*1000), (long)(sor*1000), (long)(red*1000));
    printf("%ld\n", N);
    printf("%ld\n", delta_ms);
    printf("%.5f\n", X);

    pthread_exit(NULL);

    return 0;
}
