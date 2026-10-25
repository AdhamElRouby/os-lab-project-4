
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// the functions in part 1 and part 2 
long long sequential_compute(const char *filepath,long long (*f)(long long, long long));

long long parallel_compute(char *filename,int n_proc,long long (*f)(long long, long long));


// benchmarks 
#define INPUT_FILE "numbers.txt"
#define POINTS 100  //number of different input sizes
#define REPETITIONS 50 // repeating for each N 50 times
#define FIXED_PROCESSES 4 // fixing n_proc to 4
#define MIN_N 10000LL
#define MAX_N 50000LL

// the aggregate function to test
long long add(long long a, long long b)
{
    return a + b;
}

// calculating the elapsed time in seconds
double elapsed_seconds(struct timespec start, struct timespec end)
{
    // converting the difference in seconds and nanoseconds into seconds
    return (double)(end.tv_sec - start.tv_sec)+ (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

// generating an input file containing N 1's 
int generate_input(long long N)
{
    // opening the input file in write mode, replacing its 
    FILE *file = fopen(INPUT_FILE, "w");

    // stop if the file cannot be opened
    if (file == NULL) {
        perror("Opening input file");
        return -1;
    }
    //writing N numbers to the file
    for (long long i = 0; i < N; i++) {
        if (fprintf(file, "1\n") < 0) {
            perror("Writing input file");
            fclose(file);
            return -1;
        }
    }
    //close the file 
    if (fclose(file) != 0) {
        perror("Closing input file");
        return -1;
    }
    return 0;
}

//function to verify that both functions are returning the same number 
int verify_results(long long N)
{
    long long sequential = sequential_compute(INPUT_FILE, add);

    long long parallel = parallel_compute((char *)INPUT_FILE,FIXED_PROCESSES, add); 

    // The sum is supposed to be N so we check whether they both return N
    if (sequential != N || parallel != N) 
    {
        fprintf(stderr,"ERROR: N=%lld, sequential=%lld, parallel=%lld\n",N, sequential, parallel);
        return 0; // if either one or both are wrong
    }

    return 1;
}

// measuring the execution time of the sequential
double time_sequential(void)
{
    struct timespec start, end;
    // recording the start time 
    clock_gettime(CLOCK_MONOTONIC, &start);
    // run the function
    long long result = sequential_compute(INPUT_FILE, add);
    // recording the end time
    clock_gettime(CLOCK_MONOTONIC, &end);
    (void)result;

    // use our function to get the time in secondd
    return elapsed_seconds(start, end);
}

// measuring the execution time of the parallel
double time_parallel(void)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long long result = parallel_compute((char *)INPUT_FILE,FIXED_PROCESSES,add);
    clock_gettime(CLOCK_MONOTONIC, &end);

    (void)result;

    return elapsed_seconds(start, end);
}

int main(void)
{
    // open csv file for write 
    FILE *out = fopen("part3_1_results.csv", "w");

    if (out == NULL) {
        perror("Opening results CSV");
        return 1;
    }

    //printing the headers
    if (fprintf(out, "N,n_proc,repetition,sequential_seconds,parallel_seconds\n") < 0) 
    {
        perror("Writing CSV header");
        fclose(out);
        return 1;
    }

    // ensuring the header is written before the computation
    if (fflush(out) != 0) {
        perror("Flushing CSV header");
        fclose(out);
        return 1;
    }

    //displaying the benchmarks
    printf("Part 3 (1): fixed n_proc = %d\n", FIXED_PROCESSES);
    printf("N range: %lld to %lld\n", MIN_N, MAX_N);
    printf("Input sizes: %d\n", POINTS);
    printf("Repetitions per size: %d\n", REPETITIONS);
    fflush(stdout);



    // testing 100 different input sizes from min_N to max_N 
    for (int point = 0; point < POINTS; point++) 
    {
        //calculating the current input size
        //distributing the points evenly between the min and the max 
        long long N = MIN_N + (MAX_N - MIN_N) * point / (POINTS - 1);

        // generating the input file 
        if (generate_input(N) != 0) {
            fclose(out);
            return 1;
        }

        // verifying correctness before we start
        if (!verify_results(N)) {
            fprintf(stderr,
                    "Computation verification failed for N=%lld :((( \n", N);
            fclose(out);
            return 1;
        }
        printf("\n========================================\n");
        printf("N = %lld | n_proc = %d\n",N, FIXED_PROCESSES);
        printf("========================================\n");

        double sequential_sum = 0.0;
        double parallel_sum = 0.0;
        // repeating 50 times for each N
        for (int trial = 0; trial < REPETITIONS; trial++) {
            double sequential_time;
            double parallel_time;

            // alternating the implementations: sequential -> parallel -> sequential ->parallel
            if (trial % 2 == 0) {
                sequential_time = time_sequential();
                parallel_time = time_parallel();
            } else {
                parallel_time = time_parallel();
                sequential_time = time_sequential();
            }

            // accumulating the times to calculate the average (used for initial plotting)
            sequential_sum += sequential_time;
            parallel_sum += parallel_time;

            // printing every repition
            printf("Repetition %2d | Sequential: %.9f s"" | Parallel: %.9f s\n",trial + 1,sequential_time,parallel_time);

            if (fprintf(out, "%lld,%d,%d,%.9f,%.9f\n",N,FIXED_PROCESSES,trial + 1,sequential_time,parallel_time) < 0) 
            {
                perror("Writing measurement");
                fclose(out);
                return 1;
            }
            if (fflush(out) != 0) 
            {
                perror("Flushing results CSV");
                fclose(out);
                return 1;
            }
            fflush(stdout);
        }

    // ca;culating the averages
    double sequential_avg = sequential_sum / REPETITIONS;
    double parallel_avg = parallel_sum / REPETITIONS;

    printf("----------------------------------------\n");
    printf("Average | Sequential: %.9f s"" | Parallel: %.9f s\n",sequential_avg, parallel_avg);

    // adding the average as a CSV row 
    fprintf(out, "%lld,%d,AVERAGE,%.9f,%.9f\n",N, FIXED_PROCESSES,sequential_avg, parallel_avg);
    fflush(out);

    }

    if (fclose(out) != 0) 
    {
        perror("Closing results CSV");
        return 1;
    }

    printf("\nTest 1 completed successfully :) \n");
    printf("Results have been saved to part3_1_results.csv!!\n");
    return 0;
}
