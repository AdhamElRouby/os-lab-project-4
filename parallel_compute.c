#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define N 1000000
#define N_PROC 6
#define FILE_PATH "input.txt"

int write_numbers_to_file() {
    FILE *file = fopen(FILE_PATH, "w");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    for (long long number = 1; number <= N; number++) {
        fprintf(file, "%lld\n", number);
    }

    fclose(file);
    return 0;
}

long long add(long long a, long long b) {
    return a + b;
}

int read_numbers_from_file(char* filename, long long* arr) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }

    for (int i = 0; i < N; i++) {
        fscanf(file, "%lld", &arr[i]);
    }

    fclose(file);
    return 0;
}

long long compute(long long* arr, int start, int end, long long (*f)(long long,long long)) {
    long long result = arr[start];
    for (int i = start + 1; i < end; i++) {
        result = f(result, arr[i]);
    }
    return result;
}

long long parallel_compute(char* filename, int n_proc, long long (*f)(long long,long long)) {
    if(n_proc <= 0) {
        printf("Number of processes must be greater than 0\n");
        exit(1);
    }
    if(n_proc > N) n_proc = N; // Limit the number of processes to N
    int fd[n_proc][2];
    long long arr[N];
    int div = N / n_proc;
    int rem = N % n_proc;
    int start = 0;
    int end = div;

    if(read_numbers_from_file(filename, arr) != 0) {
        printf("Error reading numbers from file\n");
        exit(1);
    }

    for(int i = 0; i < n_proc; i++) {
        if(pipe(fd[i]) == -1) {
            perror("pipe");
            exit(1);
        }
        if(i < rem) end++;
        pid_t pid = fork();
        if(pid == -1) {
            perror("fork");
            exit(1);
        }
        if(pid == 0) {
            // Child process
            close(fd[i][0]); // close read end in child
            long long result = compute(arr, start, end, f);
            if(write(fd[i][1], &result, sizeof(result)) != sizeof(result)) {
                perror("write");
                exit(1);
            }
            close(fd[i][1]); // close write end in child
            exit(0);
        } else {
            // Parent process
            close(fd[i][1]); // close write end in parent
            start = end;
            end += div;
        }
    }

    long long final_result = 0;
    for(int i = 0; i < n_proc; i++) {
        long long result;
        if(read(fd[i][0], &result, sizeof(result)) != sizeof(result)) {
            perror("read");
            exit(1);
        }
        final_result = (i == 0) ? result : f(final_result, result);
        close(fd[i][0]);
    }

    // Wait for all child processes to finish to avoid zombie processes
    for(int i = 0; i < n_proc; i++) {
        wait(NULL);
    }

    return final_result;
}

int main() {
    
    if(write_numbers_to_file() != 0) {
        printf("Error writing numbers to file\n");
        exit(1);
    }

    long long result = parallel_compute(FILE_PATH, N_PROC, add);
    printf("Final result: %lld\n", result);

    return 0;
}