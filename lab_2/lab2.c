#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <fwBase.h>
#include <fwSignal.h>

#define A 700 // Рачковский Дмитрий Эдуардович
#define NUMBER_OF_ITERATIONS 50

Fw64f random_on_interval(Fw64f min, Fw64f max, unsigned int *seed) {
    return (Fw64f) ((rand_r(seed) % (int)(max + 1 - min)) + min);
}

int main(int argc, char* argv[]) {
    struct timeval T1, T2;
    int N = atoi(argv[1]);
    int num_of_threads = atoi(argv[2]);

    Fw64f M1[N];
    Fw64f M2[N/2];
    Fw64f M2_shifted_copy[N/2];

    Fw64f X = 0;

    printf("Setting number of threads to %d\n", num_of_threads);
    FwStatus st = fwSetNumThreads(num_of_threads);
    printf("%s\n", fwGetStatusString(st));
    printf("Current number of threads: %d\n", fwGetNumThreads());

    gettimeofday(&T1, NULL);

    int i, j;
    for (i=0; i<NUMBER_OF_ITERATIONS; i++) {
        /******************** GENERATE ********************/
        unsigned int seed = i;
        for (j = 0; j < N; j++) {
            M1[j] = random_on_interval(0, A, &seed);
            // converting to radians for later computations
            M1[j] = (M1[j] * M_PI) / 180.0;
        }
        for (j = 0; j < N/2; j++) {
            M2[j] = random_on_interval(A, A*10, &seed);
            M2_shifted_copy[j] = j == 0 ? 0 : M2[j-1]; // shifted copy of M2 needed on the next step
        }

        /******************** MAP *************************/
        // operation #1
        fwsSinh_64f_A50(M1, M1, N);
        fwsPowx_64f_A50(M1, 2, M1, N);

        // operation #3
        fwsAdd_64f_I(M2_shifted_copy, M2, N/2);
        fwsTan_64f_A50(M2, M2, N/2);
        fwsAbs_64f_I(M2, N/2);

        /******************** MERGE ***********************/
        // operation #1
        fwsPow_64f_A50(M1, M2, M2, N/2);

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
                Fw64f temp = M2[min_k];
                M2[min_k] = M2[k];
                M2[k] = temp;
            }
        }

        /******************** REDUCE **********************/
        int min_index;
        for (min_index = 0; M2[min_index] <= 0; min_index++) {}
        Fw64f min = M2[min_index];

        for (j = 0; j < N/2; j++) {
            if (isfinite(M2[j]) && (int)(M2[j] / min) % 2 == 0) {
                // remember to convert to radians
                Fw64f ans = sin((M2[j] * M_PI) / 180.0);
                if (!isnan(ans)) {
                    X += ans;
                }
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
