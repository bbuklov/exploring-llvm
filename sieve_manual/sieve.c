#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void sieve(int p, unsigned char* numbers, int upper_bound) {
    int bit_index, byte_index;

    upper_bound = (int)(upper_bound / p + 1);
    for (int m = 2; m < upper_bound; m++) {
        byte_index = (int)(m * p / 8);
        bit_index = m * p % 8;
        numbers[byte_index] |= (1 << bit_index);
    }
}

int next_prime(int p, unsigned char* numbers, int size) {
    int bit_index, byte_index;
    int bi = (p + 1) % 8;

    for (byte_index = (int)((p + 1) / 8); byte_index < size; byte_index++) {
        for (bit_index = bi; bit_index < 8; bit_index++) {
            if ((numbers[byte_index] & (1 << bit_index)) == 0) {
                return byte_index * 8 + bit_index;
            }
        }
        bi = 0;
    }

    return p;
}

int print_primes(int n) {
    /*This function calculates all prime numbers up to given limit (n).
    It is an implementation of "Sieve of Eratosthenes" algorithm in C.*/

    size_t numbers_size = (size_t)(n / 8 + 1);
    unsigned char* numbers = (unsigned char*)calloc(numbers_size, sizeof(unsigned char));
    if (numbers == NULL) {
        printf("Unable to allocate memory.\n");
        exit(-1);
    }

    int p = 2;

    while (1) {
        if (p > n) {
            free(numbers);
            return 0;
        }

        sieve(p, numbers, n);
        int new_p = next_prime(p, numbers, numbers_size);

        if (p != new_p) {
            printf("%d\n", p);
            p = new_p;
        } else {
            free(numbers);
            return 0;
        }
    }

    free(numbers);
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <upper bound>.\n", argv[0]);
        exit(-1);
    }
    
    print_primes(atoi(argv[1]));

    return 0;
}