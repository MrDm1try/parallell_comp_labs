#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "ippcore.h"
#include "ippcore_tl.h"
#include "ippvm.h"
#include "ipps.h"

#define A 700 // Рачковский Дмитрий Эдуардович
#define NUMBER_OF_ITERATIONS 50

Ipp32f random_on_interval(Ipp32f min, Ipp32f max, unsigned int *seed) {
    return (Ipp32f) ((rand_r(seed) % (int)(max + 1 - min)) + min);
}

void farr_out(Ipp32f arr[], int n) {
    int j;
    for (j = 0; j < n; j++) {
        printf("%.2f ", arr[j]);
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    struct timeval T1, T2;
    Ipp32s N = atoi(argv[1]);
    int num_of_threads = atoi(argv[2]);

    ippSetNumThreads(num_of_threads);
    int bop;
    ippGetNumThreads(&bop);
    printf("%d\n", bop);

    Ipp32f M1[N];
    Ipp32f M2[N/2];
    Ipp32f M2_shifted_copy[N/2];

    double X[NUMBER_OF_ITERATIONS];

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
        // for (j = 0; j < N; j++) {
        //     // operation #1, remember to convert to radians
        //     M1[j] = pow(sinhl((M1[j] * M_PI) / 180.0), 2);
        // }

        // operation #1
        ippsSinh_32f_A24(M1, M1, N);
        ippsPowx_32f_A24(M1, 2, M1, N);

        // for (j = 0; j < N/2; j++) {
        //     // operation #3
        //     M2[j] = fabs(tanl(M2[j] + M2_shifted_copy[j]));
        // }

        // operation #3
        ippsAdd_32f_I(M2_shifted_copy, M2, N/2);
        ippsTan_32f_A24(M2, M2, N/2);
        ippsAbs_32f_A24(M2, M2, N/2);

        /******************** MERGE ***********************/
        // for (j = 0; j < N/2; j++) {
        //     // operation #1
        //     M2[j] = pow(M1[j], M2[j]);
        // }

        // operation #1
        ippsPow_32f_A24(M1, M2, M2, N/2);

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
                Ipp32f temp = M2[min_k];
                M2[min_k] = M2[k];
                M2[k] = temp;
            }
        }

        /******************** REDUCE **********************/
        int min_index;
        for (min_index = 0; M2[min_index] <= 0; min_index++) {}
        Ipp32f min = M2[min_index];

        X[i] = 0;
        for (j = 0; j < N/2; j++) {
            if (isfinite(M2[j]) && (int)(M2[j] / min) % 2 == 0) {
                // remember to convert to radians
                X[i] += sinl((M2[j] * M_PI) / 180.0);
            }
        }
    }

    gettimeofday(&T2, NULL);
    long delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("%d\n", N);
    printf("%ld\n", delta_ms);
    for (i = 0; i < NUMBER_OF_ITERATIONS; i++) {
        printf("%.5f\n", X[i]);
    }
    return 0;
}
