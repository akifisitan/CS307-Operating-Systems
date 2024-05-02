#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// Helper for printing depth
void printDepth(int depth, int lr) {
    char* dashes = malloc(depth * 3 + 1);
    for (int i = 0; i < depth * 3; i++) {
        dashes[i] = '-';
    }
    dashes[depth * 3] = '\0';
    fprintf(stderr, "%s> current depth: %d, lr: %d\n", dashes, depth, lr);
    free(dashes);
}

// Helper for printing depth alongside num1 and num2
void printFullDepth(int depth, int lr, int num1, int num2) {
    char* dashes = malloc(depth * 3 + 1);
    for (int i = 0; i < depth * 3; i++) {
        dashes[i] = '-';
    }
    dashes[depth * 3] = '\0';
    fprintf(stderr, "%s> current depth: %d, lr: %d, my num1: %d, my num2: %d\n", dashes, depth, lr, num1, num2);
    free(dashes);
}

// Helper for printing result or num1 depending on useCase
void printResult(int depth, int lr, int useCase, int num) {
    char* dashes = malloc(depth * 3 + 1);
    for (int i = 0; i < depth * 3; i++) {
        dashes[i] = '-';
    }
    dashes[depth * 3] = '\0';
    if (useCase == 1) {
        fprintf(stderr, "%s> my num1 is: %d\n", dashes, num);
    } 
    else {
        fprintf(stderr, "%s> my result is: %d\n", dashes, num);
    }
    free(dashes);
}

int isRootNode(int curDepth) {
    return curDepth == 0;
}

int isLeafNode(int curDepth, int maxDepth) {
    return curDepth == maxDepth;
}

int main(int argc, char* argv[]) {

    // Check args
    if (argc != 4) {
        fprintf(stderr, "Usage: treePipe <current depth> <max depth> <left-right>\n");
        return 0;
    }
    // Parse command line arguments
    int curDepth = atoi(argv[1]);
    int maxDepth = atoi(argv[2]);
    int lr = atoi(argv[3]);

    printDepth(curDepth, lr);

    // Display prompt if root node
    if (isRootNode(curDepth)) {
        fprintf(stderr, "Please enter num1 for the root: ");
    }
    // Num1 will either be entered by the user if root node or read from pipe via stdin
    int num1;
    scanf("%d", &num1);

    printResult(curDepth, lr, 1, num1);

    // Do not create left and right children, just call worker process in the case of a leaf node
    if (isLeafNode(curDepth, maxDepth)) {

        // Create pipes for communication with worker process
        int input_pipe_worker[2], output_pipe_worker[2];
        if (pipe(input_pipe_worker) == -1 || pipe(output_pipe_worker) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Create worker process
        int pid_worker = fork();
        if (pid_worker == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        // Leaf node worker process
        if (pid_worker == 0) {
            close(input_pipe_worker[1]); // Close unused write end of input pipe
            close(output_pipe_worker[0]); // Close unused read end of output pipe

            // Redirect stdin from input pipe read end
            if (dup2(input_pipe_worker[0], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            // Redirect stdout to output pipe write end
            if (dup2(output_pipe_worker[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            // Run left or right program depending on lr
            if (lr == 0) {
                char* args[] = { "./left", NULL };
                execvp(args[0], args);
                // Execvp only returns in case of error
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            char* args[] = { "./right", NULL };
            execvp(args[0], args);
            // Execvp only returns in case of error
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        // Leaf node process
        close(input_pipe_worker[0]); // Close unused read end of input pipe
        close(output_pipe_worker[1]); // Close unused write end of output pipe

        // Write input num1 and 1 (default num2 for leaf nodes) to worker leaf child process
        char worker_input[50];
        sprintf(worker_input, "%d\n1\n", num1);
        write(input_pipe_worker[1], worker_input, strlen(worker_input));
        close(input_pipe_worker[1]); // Close write end of input pipe after writing

        // Wait for worker child process to finish
        waitpid(pid_worker, NULL, 0);

        // Read output (res) from worker child process
        char worker_output[11];
        int res;
        int bytes_read_worker = read(output_pipe_worker[0], worker_output, sizeof(worker_output) - 1);
        if (bytes_read_worker >= 0) {
            worker_output[bytes_read_worker] = '\0';
            res = atoi(worker_output);
        }
        else {
            perror("read");
        }
        close(output_pipe_worker[0]); // Close read end of output pipe after reading

        // Print result
        printResult(curDepth, lr, 0, res);

        // If leaf node is also the root node, print the final result
        if (isRootNode(curDepth)) {
            fprintf(stderr, "The final result is: %d\n", res);
        }
        // Leaf node computation complete, send output to parent via stdout
        else {
            printf("%d\n", res);
        }
        return 0;
    }

    // Process continues if not leaf node

    // Create pipes to communicate with left child process
    int input_pipe_left[2], output_pipe_left[2];
    if (pipe(input_pipe_left) == -1 || pipe(output_pipe_left) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Create left child
    int pid_left = fork();
    if (pid_left == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Left child process
    if (pid_left == 0) {
        // fprintf(stderr, "In left child process\n");
        close(input_pipe_left[1]); // Close unused write end of input pipe
        close(output_pipe_left[0]); // Close unused read end of output pipe

        // Redirect stdin from input pipe read end
        if (dup2(input_pipe_left[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to output pipe write end
        if (dup2(output_pipe_left[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Convert args from int to string
        char curDepthStr[50];
        char maxDepthStr[50];
        sprintf(curDepthStr, "%d", curDepth + 1);
        sprintf(maxDepthStr, "%d", maxDepth);

        // Call execvp with main program for depth + 1 and left (lr=0)
        char* args[] = { argv[0], curDepthStr, maxDepthStr, "0", NULL };
        execvp(args[0], args);
        // Execvp only returns on error
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(input_pipe_left[0]); // Close unused read end of input pipe
    close(output_pipe_left[1]); // Close unused write end of output pipe

    // Write input (num1) to pipe, left child process will read it through stdin (via scanf)
    char left_input[50];
    sprintf(left_input, "%d\n", num1);
    write(input_pipe_left[1], left_input, strlen(left_input));
    close(input_pipe_left[1]);

    // wait for left child process to finish
    waitpid(pid_left, NULL, 0);

    // Read output (num2) from left child
    char left_output[11];
    int num2;
    int bytes_read_left = read(output_pipe_left[0], left_output, sizeof(left_output) - 1);
    if (bytes_read_left >= 0) {
        left_output[bytes_read_left] = '\0';
        num2 = atoi(left_output);
    }
    else {
        perror("read");
    }
    close(output_pipe_left[0]);

    // num2 obtained, next step: calculation with worker process

    // Create pipes for communication with worker process
    int input_pipe_worker[2], output_pipe_worker[2];
    if (pipe(input_pipe_worker) == -1 || pipe(output_pipe_worker) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Create worker process
    int pid_worker = fork();
    if (pid_worker == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Worker process
    if (pid_worker == 0) {
        // fprintf(stderr, "In worker process\n");
        close(input_pipe_worker[1]); // Close unused write end of input pipe
        close(output_pipe_worker[0]); // Close unused read end of output pipe

        // Redirect stdin from input pipe read end
        if (dup2(input_pipe_worker[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to output pipe write end
        if (dup2(output_pipe_worker[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Run left or right program depending on lr
        if (lr == 0) {
            char* args[] = { "./left", NULL };
            execvp(args[0], args);
            // Execvp only returns in case of error
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        char* args[] = { "./right", NULL };
        execvp(args[0], args);
        // Execvp only returns in case of error
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(input_pipe_worker[0]); // Close unused read end of input pipe
    close(output_pipe_worker[1]); // Close unused write end of output pipe

    // Write num1 and num2 inputs to pipe, worker process reads them through stdin
    char worker_input[50];
    sprintf(worker_input, "%d\n%d\n", num1, num2);

    write(input_pipe_worker[1], worker_input, strlen(worker_input));
    close(input_pipe_worker[1]);

    // Wait for worker process to finish
    waitpid(pid_worker, NULL, 0);

    // Read worker process output (res)
    char worker_output[11];
    int res;
    int bytes_read_worker = read(output_pipe_worker[0], worker_output, sizeof(worker_output) - 1);
    if (bytes_read_worker >= 0) {
        worker_output[bytes_read_worker] = '\0';
        res = atoi(worker_output);
    }
    else {
        perror("read");
    }
    close(output_pipe_worker[0]);

    // Print worker process results
    printFullDepth(curDepth, lr, num1, num2);
    printResult(curDepth, lr, 0, res);

    // res obtained, next step: process right children

    // Create pipes for communicating with right child process
    int input_pipe_right[2], output_pipe_right[2];
    if (pipe(input_pipe_right) == -1 || pipe(output_pipe_right) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Create right child process
    int pid_right = fork();
    if (pid_right == -1) {
        exit(EXIT_FAILURE);
    }

    // Right child process
    if (pid_right == 0) {
        // fprintf(stderr, "In right child process\n");
        close(input_pipe_right[1]); // Close unused write end of input pipe
        close(output_pipe_right[0]); // Close unused read end of output pipe

        // Redirect stdin from input pipe read end
        if (dup2(input_pipe_right[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to output pipe write end
        if (dup2(output_pipe_right[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Convert args from int to string
        char curDepthStr[50];
        char maxDepthStr[50];
        sprintf(curDepthStr, "%d", curDepth + 1);
        sprintf(maxDepthStr, "%d", maxDepth);

        // Call execvp with main program for depth + 1 and right (lr=1)
        char* args[] = { argv[0], curDepthStr, maxDepthStr, "1", NULL };
        execvp(args[0], args);
        // Execvp only returns on error
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Parent process
    close(input_pipe_right[0]); // Close unused read end of input pipe
    close(output_pipe_right[1]); // Close unused write end of output pipe

    // Write input (res) to right child process
    char right_input[50];
    sprintf(right_input, "%d\n", res);

    write(input_pipe_right[1], right_input, strlen(right_input));
    close(input_pipe_right[1]);

    // wait for right child process to finish
    waitpid(pid_right, NULL, 0);

    // Read output from right child
    char right_output[11];
    int final_result;
    int bytes_read_right = read(output_pipe_right[0], right_output, sizeof(right_output) - 1);
    if (bytes_read_right >= 0) {
        right_output[bytes_read_right] = '\0';
        final_result = atoi(right_output);
    }
    else {
        perror("read");
    }
    close(output_pipe_left[0]);

    // Print final result if root node 
    if (isRootNode(curDepth)) {
        fprintf(stderr, "The final result is: %d\n", final_result);
    }
    // Otherwise send the result upwards to parent of current process via stdout, which will read via stdin
    else {
        printf("%d\n", final_result);
    }
    return 0;
}
