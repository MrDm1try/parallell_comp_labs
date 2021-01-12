#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define A 700 // Рачковский Дмитрий Эдуардович
#define NUMBER_OF_ITERATIONS 50

typedef enum stage_ {
    STAGE_UNDEFINED = 0,
    STAGE_GENERATE = 0,
    STAGE_MAP,
    STAGE_MERGE,
    STAGE_SORT,
    STAGE_REDUCE,
    STAGE_NUM_OF,
} stage_t;

int current_iteration = 0;
stage_t current_stage = STAGE_UNDEFINED;

void monitor_execution_percent();

#ifdef _OPENMP
    #include "omp.h"
    void monitor_execution_percent() {
        while (1) {
            sleep(1);
            double execution_percent = (100. / NUMBER_OF_ITERATIONS) * current_iteration + (100. / NUMBER_OF_ITERATIONS) * ((double)current_stage / STAGE_NUM_OF);
            if (execution_percent < 100) {
                printf("Current progress: %.2f%%\n", execution_percent);
            } else {
                break;
            }
        };
    }
#else
    #include <sys/time.h>
    double omp_get_wtime() { struct timeval T; gettimeofday(&T, NULL); return T.tv_sec + T.tv_usec / 1000000.; }
    int omp_get_num_procs() { return 1; }
    int omp_get_thread_num() { return 0; }
    void omp_set_nested(int n) {}
    void monitor_execution_percent() {}
#endif

long double random_on_interval(long double min, long double max, unsigned int *seed) {
    return (long double) ((rand_r(seed) % (int)(max + 1 - min)) + min);
}

unsigned int make_seed(int i, int j) {
    return 9572 + 234*i + 456*j;
}

int main(int argc, char* argv[]) {
    int N = atoi(argv[1]);

    omp_set_nested(1);

    double T1, T2, last_time_measure;
    double gen = 0;
    double map = 0;
    double mer = 0;
    double sor = 0;
    double red = 0;
    long double M1[N];
    long double M2[N/2];
    long double M2_copy[N/2];

    double X = 0;

    int j = 0;
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            monitor_execution_percent();
        }
        #pragma omp section
        {
            T1 = omp_get_wtime();
            last_time_measure = T1;
            for (current_iteration=0; current_iteration<NUMBER_OF_ITERATIONS; current_iteration++) {
                /******************** GENERATE ********************/
                current_stage = STAGE_GENERATE;
                unsigned int seed;

                #pragma omp parallel for default(none) private(j, seed) shared(M1, N, current_iteration)
                for (j = 0; j < N; j++) {
                    seed = make_seed(current_iteration, j);
                    M1[j] = random_on_interval(0, A, &seed);
                }

                #pragma omp parallel for default(none) private(j, seed) shared(M2, N, current_iteration)
                for (j = 0; j < N/2; j++) {
                    seed = make_seed(current_iteration, j);
                    M2[j] = random_on_interval(A, A*10, &seed);
                }
                // make a copy, shifting M2 one element to the right
                M2_copy[0] = 0;
                memcpy(&M2_copy[1], M2, sizeof(long double) * ((N/2)-1));

                gen += omp_get_wtime() - last_time_measure;
                last_time_measure = omp_get_wtime();
                /******************** MAP *************************/
                current_stage = STAGE_MAP;
                #pragma omp parallel for default(none) private(j) shared(M1, N)
                for (j = 0; j < N; j++) {
                    // operation #1, remember to convert to radians
                    M1[j] = pow(sinhl((M1[j] * M_PI) / 180.0), 2);
                }

                #pragma omp parallel for default(none) private(j) shared(M2, M2_copy, N)
                for (j = 0; j < N/2; j++) {
                    // operation #3
                    M2[j] = fabs(tanl(M2[j] + M2_copy[j]));
                }

                map += omp_get_wtime() - last_time_measure;
                last_time_measure = omp_get_wtime();
                /******************** MERGE ***********************/
                current_stage = STAGE_MERGE;
                #pragma omp parallel for default(none) private(j) shared(M1, M2, N)
                for (j = 0; j < N/2; j++) {
                    // operation #1
                    M2[j] = pow(M1[j], M2[j]);
                }

                mer += omp_get_wtime() - last_time_measure;
                last_time_measure = omp_get_wtime();
                /******************** SORT ************************/
                current_stage = STAGE_SORT;
                int elem_per_part = ((N/2) / omp_get_num_procs()) + 1;
                int start_locations[omp_get_num_procs()];
                #pragma omp parallel
                {
                    int start_inc = elem_per_part * omp_get_thread_num();
                    int finish_non_inc = (start_inc + elem_per_part) < N/2 ? start_inc + elem_per_part : N/2;
                    start_locations[omp_get_thread_num()] = start_inc;
                    for (int k = start_inc; k < finish_non_inc-1; k++)
                    {
                        int min_k = k;
                        for (int l = k+1; l < finish_non_inc; l++)  {
                            if (M2[l] < M2[min_k]) {
                                min_k = l;
                            }
                        }
                        if (min_k != k) {
                            long double temp = M2[k];
                            M2[k] = M2[min_k];
                            M2[min_k] = temp;
                        }
                    }
                }
                // we'll be reusing this variable
                memcpy(M2_copy, M2, sizeof(long double) * (N/2));
                for (j = 0; j < N/2; j++) {
                    int min_interval = 0;
                    for (int m = 1; m < omp_get_num_procs(); m++) {
                        if ((start_locations[m] < N/2) && (M2_copy[start_locations[m]] < M2_copy[start_locations[min_interval]])) {
                            min_interval = m;
                        }
                    }
                    M2[j] = M2_copy[start_locations[min_interval]];
                    M2_copy[start_locations[min_interval]] = INFINITY;
                    start_locations[min_interval]++;
                }

                sor += omp_get_wtime() - last_time_measure;
                last_time_measure = omp_get_wtime();
                /******************** REDUCE **********************/
                current_stage = STAGE_REDUCE;
                int min_index;
                for (min_index = 0; M2[min_index] <= 0; min_index++) {}
                long double min = M2[min_index];

                #pragma omp parallel for default(none) private(j) shared(M2, min, N, current_iteration) reduction(+:X)
                for (j = 0; j < N/2; j++) {
                    if (isfinite(M2[j]) && (int)(M2[j] / min) % 2 == 0) {
                        // remember to convert to radians
                        X += sinl((M2[j] * M_PI) / 180.0);
                    }
                }
                red += omp_get_wtime() - last_time_measure;
                last_time_measure = omp_get_wtime();
                current_stage = STAGE_UNDEFINED;
            }
            T2 = omp_get_wtime();
        }
    }

    long delta_ms = (T2 - T1) * 1000;
    printf("%ld\n%ld\n%ld\n%ld\n%ld\n", (long)(gen*1000), (long)(map*1000), (long)(mer*1000), (long)(sor*1000), (long)(red*1000));
    printf("%d\n", N);
    printf("%ld\n", delta_ms);
    printf("%.5f\n", X);

    return 0;
}
