
/* Enable POSIX functions, including clock_gettime() */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Functions implemented in Part1.c and Part2.c */
long long sequential_compute(
    const char *filepath,
    long long (*f)(long long, long long)
);

long long parallel_compute(
    char *filename,
    int n_proc,
    long long (*f)(long long, long long)
);


// benchmarks 

#define INPUT_FILE "numbers.txt"
#define FIXED_N 50000LL       // keeping N constant
#define MIN_PROCESSES 1       // Min processes is 1
#define MAX_PROCESSES 15       // testing until 15 processes
#define REPETITIONS 50        // repeating 50 times for each N

// the aggregate function to test
long long add(long long a, long long b)
{
    return a + b;
}

// calculating the elapsed time in seconds
double elapsed_seconds(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

// generating an input file containing N 1's 
int generate_input(void)
{
    FILE *file = fopen(INPUT_FILE, "w");
    if (file == NULL) 
    {
        perror("Opening input file");
        return -1;
    }

    //writing N numbers to the file
    for (long long i = 0; i < FIXED_N; i++) 
    {
        if (fprintf(file, "1\n") < 0) 
        {
            perror("Writing input file");
            fclose(file);
            return -1;
        }
    }

    if (fclose(file) != 0) 
    {
        perror("Closing input file");
        return -1;
    }

    return 0;
}

//function to verify that both functions are returning the same number 
int verify_results(int n_proc)
{
    long long sequential = sequential_compute(INPUT_FILE, add);
    long long parallel = parallel_compute((char *)INPUT_FILE,n_proc,add);

    // The sum is supposed to be N so we check whether they both return N
    if (sequential != FIXED_N || parallel != FIXED_N) {
        fprintf(stderr,"ERROR: n_proc=%d, sequential=%lld, parallel=%lld\n",n_proc, sequential, parallel);
        return 0;
    }
    return 1;
}

// measuring the execution time of the sequential
double time_sequential(void)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long long result = sequential_compute(INPUT_FILE, add);
    clock_gettime(CLOCK_MONOTONIC, &end);
    (void)result;
    return elapsed_seconds(start, end);
}

// measuring the execution time of the parallel
double time_parallel(int n_proc)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long long result = parallel_compute((char *)INPUT_FILE, n_proc,add);
    clock_gettime(CLOCK_MONOTONIC, &end);
    (void)result;
    return elapsed_seconds(start, end);
}


int main(void)
{
    // creating a CSV file for part 2
    FILE *out = fopen("part3_2_results.csv", "w");

    if (out == NULL) 
    {
        perror("Opening results CSV");
        return 1;
    }

    if (fprintf(out,"N,n_proc,repetition,sequential_seconds,parallel_seconds\n") < 0) 
    {
        perror("Writing CSV header");
        fclose(out);
        return 1;
    }

    // generating the input file once
    if (generate_input() != 0) {
        fclose(out);
        return 1;
    }

    printf("Part 3 (2): fixed N = %lld\n", FIXED_N);
    printf("Processes tested: %d to %d\n",MIN_PROCESSES, MAX_PROCESSES);
    printf("Repetitions per process count: %d\n", REPETITIONS);

    // increasing the number of processes
    for (int n_proc = MIN_PROCESSES; n_proc <= MAX_PROCESSES; n_proc++) {

        // confirming correctness
        if (!verify_results(n_proc)) {
            fprintf(stderr,"Verification failed for n_proc=%d\n", n_proc);
            fclose(out);
            return 1;
        }
        printf("\n========================================\n");
        printf("N = %lld | n_proc = %d\n", FIXED_N, n_proc);
        printf("========================================\n");

        double sequential_sum = 0.0;
        double parallel_sum = 0.0;

        for (int trial = 0; trial < REPETITIONS; trial++) {

            double sequential_time;
            double parallel_time;

            // alternating
            if (trial % 2 == 0) {
                sequential_time = time_sequential();
                parallel_time = time_parallel(n_proc);
            } else 
            {
                parallel_time = time_parallel(n_proc);
                sequential_time = time_sequential();
            }

            // for the averages
            sequential_sum += sequential_time;
            parallel_sum += parallel_time;

            /* Print this repetition's results */
            printf("Repetition %2d | Sequential: %.9f s"" | Parallel: %.9f s\n",trial + 1, sequential_time, parallel_time);

            // saving individual measurements to the csv
            if (fprintf(out, "%lld,%d,%d,%.9f,%.9f\n",FIXED_N, n_proc, trial + 1,sequential_time, parallel_time) < 0)
             {
                perror("Writing measurement");
                fclose(out);
                return 1;
            }
            fflush(out);
        }
        double sequential_avg = sequential_sum / REPETITIONS;
        double parallel_avg = parallel_sum / REPETITIONS;

        printf("----------------------------------------\n");
        printf("Average | Sequential: %.9f s"" | Parallel: %.9f s\n", sequential_avg, parallel_avg);

        // adding average row
        if (fprintf(out, "%lld,%d,AVERAGE,%.9f,%.9f\n",FIXED_N, n_proc,sequential_avg, parallel_avg) < 0)
         {
            perror("Writing average");
            fclose(out);
            return 1;
        }
        fflush(out);
    }
    if (fclose(out) != 0) 
    {
        perror("Closing results CSV");
        return 1;
    }

    printf("\nTest 2 completed successfully :) \n");
    printf("Results saved to part3_2_results.csv!!\n");
    return 0;
}
