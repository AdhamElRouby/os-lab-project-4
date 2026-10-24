#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

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
        fprintf(file, "%d\n", 1);
    }

    fclose(file);
    return 0;
}

long long add(long long a, long long b) {
    return a + b;
}


// PARALLEL_COMPTUE STARTS HERE

#define BUF_SIZE (1 << 16)
// Pass 1: count the numbers (one per line) without parsing them.
long long count_lines(char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }
    char buf[BUF_SIZE];
    size_t got;
    long long lines = 0;
    while ((got = fread(buf, 1, BUF_SIZE, file)) > 0) {
        for (size_t j = 0; j < got; j++) {
            if (buf[j] == '\n') lines++;
        }
    }
    fclose(file);
    return lines;
}

// Pass 2: offsets[i] = byte where line starts[i] begins.
void find_offsets(char* filename, long long* starts, off_t* offsets, int n_proc) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }
    char buf[BUF_SIZE];
    size_t got;
    long long line = 0;
    off_t pos = 0;
    int next = 1;
    offsets[0] = 0;
    while (next < n_proc && (got = fread(buf, 1, BUF_SIZE, file)) > 0) {
        for (size_t j = 0; j < got && next < n_proc; j++) {
            if (buf[j] == '\n') {
                line++;
                // `while`: children with 0 numbers share the same start line
                while (next < n_proc && line == starts[next]) offsets[next++] = pos + j + 1;
            }
        }
        pos += got;
    }
    fclose(file);
}

// Folds `count` numbers starting at byte `offset` of the file.
long long compute(char* filename, off_t offset, long long count, long long (*f)(long long,long long)) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }
    fseeko(file, offset, SEEK_SET);
    long long result, x;
    fscanf(file, "%lld", &result);
    for (long long i = 1; i < count; i++) {
        fscanf(file, "%lld", &x);
        result = f(result, x);
    }
    fclose(file);
    return result;
}

long long parallel_compute(char* filename, int n_proc, long long (*f)(long long,long long)) {
    if(n_proc <= 0) {
        printf("Number of processes must be greater than 0\n");
        exit(1);
    }
    long long n = count_lines(filename);
    if(n == 0) {
        printf("File is empty\n");
        exit(1);
    }
    int fd[n_proc][2];
    long long starts[n_proc];
    long long counts[n_proc];
    off_t offsets[n_proc];
    long long div = n / n_proc;
    long long rem = n % n_proc;
    long long start = 0;

    for(int i = 0; i < n_proc; i++) {
        counts[i] = div + (i < rem);
        starts[i] = start;
        start += counts[i];
    }
    find_offsets(filename, starts, offsets, n_proc);

    fflush(stdout); // so children don't re-print the parent's buffered output
    for(int i = 0; i < n_proc; i++) {
        if(pipe(fd[i]) == -1) {
            perror("pipe");
            exit(1);
        }
        pid_t pid = fork();
        if(pid == -1) {
            perror("fork");
            exit(1);
        }
        if(pid == 0) {
            // Child process
            close(fd[i][0]); // close read end in child
            if(counts[i] > 0) { // a child with no numbers sends nothing
                long long result = compute(filename, offsets[i], counts[i], f);
                if(write(fd[i][1], &result, sizeof(result)) != sizeof(result)) {
                    perror("write");
                    exit(1);
                }
            }
            close(fd[i][1]); // close write end in child
            exit(0);
        } else {
            // Parent process
            close(fd[i][1]); // close write end in parent
        }
    }

    long long final_result = 0;
    for(int i = 0; i < n_proc; i++) {
        if(counts[i] == 0) { // empty child: nothing to read
            close(fd[i][0]);
            continue;
        }
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

// PARALLEL_COMPTUE ENDS HERE


int main() {

    if(write_numbers_to_file() != 0) {
        printf("Error writing numbers to file\n");
        exit(1);
    }

    long long result = parallel_compute(FILE_PATH, N_PROC, add);
    printf("Final result: %lld\n", result);

    return 0;
}
