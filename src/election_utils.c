#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "election_utils.h"

int seeded = 0;

double get_random_double() {
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }
    double random_num = (double)rand() / RAND_MAX;
    printf("Random number: %f\r\n", random_num);
    return random_num;
}

double get_threshold(double threshold) {
    return threshold;
}