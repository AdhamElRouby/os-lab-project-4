#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// goal: reading numbers from a file and combining them sequentially using a function f


// the function takes as parameters the file and a pointer to a function(f) which takes two long long values and returns a long long as well
// this function returns the final aggregated value
long long sequential_compute(const char *filepath,long long (*f)(long long, long long)) 
{
    FILE *file = fopen(filepath, "r");
    if (file == NULL) 
    {
        perror("fopen");
        return 0; //returning 0 if it fails to open 
    }

    // dynamic array to store the input numbers
    long long *numbers = NULL;
    int count = 0; // number of elements in the array
    int capacity = 0;
    char line[1024]; // temporary buffer that reads one line at a time


    // looping line by line as long as we havent reached the end 
    while (fgets(line, sizeof(line), file) != NULL) {
        char *token = strtok(line, ", \t\r\n"); // split the line into tokens using commas and whitespaces ( to be able to support different ways of input)

        // memory allocation
        //we check if the array has enough space
        //initially we let the capacity = 16 
        // then , everytime the count reaches the capacity of the array, we double its size :)
        while (token != NULL) {
            if (count == capacity) 
            {
                capacity = (capacity == 0) ? 16 : capacity * 2;

                // calculating the actual required memory size in bytes and allocate
                long long *temp = realloc(numbers,(size_t)capacity * sizeof(long long));

                // checking if the allocation failed 
                if (temp == NULL) 
                {
                    free(numbers); // release previously allocated memory
                    fclose(file);
                    return 0; // exit the functon
                }
                // otherwise, update the array pointer to the new allocated memory block
                numbers = temp; 
            }

            //converting the token from a string to long long and storing it at the index  
            numbers[count++] = atoll(token);
            token = strtok(NULL, ", \t\r\n"); // getting the next token & null tells the function to contniue where it left off 
        }
    }


    // close the file
    fclose(file);
    if (count == 0) 
    {
        free(numbers); // release the dynamically allocated array
        return 0;
    }

    // initialize the result with the first number in the array
    long long result = numbers[0];


    // apply aggregate function f to the result and each remaining number
    for (int i = 1; i < count; i++) 
    {
        result = f(result, numbers[i]);
    }

    free(numbers);
    return result;
}

/*
Example usage:

long long add(long long a, long long b) {
    return a + b;
}

int main(void) {
    long long result = sequential_compute("numbers.txt", add);
    printf("Result = %lld\n", result);
    return 0;
}
*/