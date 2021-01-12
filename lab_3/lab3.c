#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define A 700 // Рачковский Дмитрий Эдуардович
#define NUMBER_OF_ITERATIONS 50

long double random_on_interval(long double min, long double max, unsigned int *seed) {
    return (long double) ((rand_r(seed) % (int)(max + 1 - min)) + min);
}

unsigned int make_seed(int i, int j) {
    return 9572 + 234*i + 456*j;
}

int main(int argc, char* argv[]) {
    struct timeval T1, T2;
    int N = atoi(argv[1]);

    long double M1[N];
    long double M2[N/2];
    long double M2_shifted_copy[N/2];

    double X = 1;

    gettimeofday(&T1, NULL);

    int i, j = 0;
    //#pragma omp parallel for default(none) private(i, j, M1, M2, M2_shifted_copy) shared(N) schedule(runtime) reduction(+:X)
    for (i=0; i<NUMBER_OF_ITERATIONS; i++) {
        /******************** GENERATE ********************/
        unsigned int seed;

        #pragma omp parallel for default(none) private(j, seed) shared(M1, N, i) schedule(runtime)
        for (j = 0; j < N; j++) {
            seed = make_seed(i, j);
            M1[j] = random_on_interval(0, A, &seed);
        }

        #pragma omp parallel for default(none) private(j, seed) shared(M2, N, M2_shifted_copy, i) schedule(runtime)
        for (j = 0; j < N/2; j++) {
            seed = make_seed(i, j);
            M2[j] = random_on_interval(A, A*10, &seed);
        }
        M2_shifted_copy[0] = 0;
        memcpy(&M2_shifted_copy[1], M2, sizeof(long double) * ((N/2)-1));

        /******************** MAP *************************/
        #pragma omp parallel for default(none) private(j) shared(M1, N) schedule(runtime)
        for (j = 0; j < N; j++) {
            // operation #1, remember to convert to radians
            M1[j] = pow(sinhl((M1[j] * M_PI) / 180.0), 2);
        }

        #pragma omp parallel for default(none) private(j) shared(M2, M2_shifted_copy, N) schedule(runtime)
        for (j = 0; j < N/2; j++) {
            // operation #3
            M2[j] = fabs(tanl(M2[j] + M2_shifted_copy[j]));
        }

        /******************** MERGE ***********************/
        #pragma omp parallel for default(none) private(j) shared(M1, M2, N) schedule(runtime)
        for (j = 0; j < N/2; j++) {
            // operation #1
            M2[j] = pow(M1[j], M2[j]);
        }

        /******************** SORT ************************/
        int k, l, min_k;
        for (k = 0; k < (N/2)-1; k++)
        {
            min_k = k;
            for (l = k+1; l < N/2; l++)  {
                if (M2[l] < M2[min_k]) {
                    min_k = l;
                }
            }
            if (min_k != k) {
                long double temp = M2[min_k];
                M2[min_k] = M2[k];
                M2[k] = temp;
            }
        }

        /******************** REDUCE **********************/
        int min_index;
        for (min_index = 0; M2[min_index] <= 0; min_index++) {}
        long double min = M2[min_index];

        #pragma omp parallel for default(none) private(j) shared(M2, min, N) reduction(+:X) schedule(runtime)
        for (j = 0; j < N/2; j++) {
            if (isfinite(M2[j]) && (int)(M2[j] / min) % 2 == 0) {
                // remember to convert to radians
                X += sinl((M2[j] * M_PI) / 180.0);
            }
        }
    }

    gettimeofday(&T2, NULL);
    long delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("%d\n", N);
    printf("%ld\n", delta_ms);
    printf("%.5f\n", X);

    return 0;
}
