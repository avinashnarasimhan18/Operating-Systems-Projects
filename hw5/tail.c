#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

#define BUFSIZE 512

void *safe_sbrk(int n) {
    void *ptr = sbrk(n);
    if (ptr == (void *)-1) {
        printf(2, "tail: memory allocation failed\n");
        exit();
    }
    return ptr;
}

void tail(int fd, int num_lines) {
    char buf[BUFSIZE];
    char **lines = (char **)safe_sbrk(128 * sizeof(char *)); // Dynamic array for line pointers
    int line_count = 0;
    int line_capacity = 128;
    int size, i, j;
    int last_line_incomplete = 0; // Track if the last line was incomplete

    // Read input and store lines dynamically
    char *line_start = (char *)safe_sbrk(BUFSIZE); // Allocate memory for the first chunk
    char *current = line_start;
    int content_capacity = BUFSIZE;

    while ((size = read(fd, buf, BUFSIZE)) > 0) {
        for (i = 0; i < size; i++) {
            *current = buf[i];
            if (*current == '\n') {
                // Null-terminate the line
                *current = '\0';

                // Store line pointer
                if (line_count == line_capacity) {
                    lines = (char **)safe_sbrk(line_capacity * sizeof(char *));
                    line_capacity *= 2;
                }
                lines[line_count++] = line_start;

                // Allocate memory for the next line
                line_start = current + 1;
                int offset = line_start - (char *)sbrk(0);
                if (offset >= 0 || offset + BUFSIZE >= content_capacity) {
                    sbrk(BUFSIZE);
                    content_capacity += BUFSIZE;
                }
                current = line_start;
                last_line_incomplete = 0;
            } else {
                current++;
                last_line_incomplete = 1;
            }
        }
    }

    if (size < 0) {
        printf(2, "tail: read error\n");
        exit();
    }

    // Handle the case where the input doesn't end with a newline
    if (last_line_incomplete && current > line_start) {
        *current = '\0'; // Null-terminate the last line
        if (line_count == line_capacity) {
            lines = (char **)safe_sbrk(line_capacity * sizeof(char *));
            line_capacity *= 2;
        }
        lines[line_count++] = line_start;
    }

    // Output the last `num_lines` lines
    int start_index = line_count > num_lines ? line_count - num_lines : 0;
    for (j = start_index; j < line_count; j++) {
        printf(1, "%s", lines[j]);
        if (j != line_count - 1 || last_line_incomplete == 0) {
            printf(1, "\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int num_lines = 10; // Default to last 10 lines
    int fd = 0;         // Default to stdin if no file provided

    if (argc >= 2 && strcmp(argv[1], "-n") == 0) {
        // Handle -n NUM
        if (argc >= 3) {
            num_lines = atoi(argv[2]);
            if (num_lines <= 0) {
                printf(2, "tail: invalid number of lines\n");
                exit();
            }
            if (argc == 4) {
                fd = open(argv[3], 0);
                if (fd < 0) {
                    printf(2, "tail: cannot open file %s\n", argv[3]);
                    exit();
                }
            }
        } else {
            printf(2, "Usage: tail -n NUM [file]\n");
            exit();
        }
    } else if (argc >= 2 && argv[1][0] == '-') {
        // Handle -NUM
        num_lines = atoi(argv[1] + 1);
        if (num_lines <= 0) {
            printf(2, "tail: invalid number of lines\n");
            exit();
        }
        if (argc == 3) {
            fd = open(argv[2], 0);
            if (fd < 0) {
                printf(2, "tail: cannot open file %s\n", argv[2]);
                exit();
            }
        }
    } else if (argc == 2) {
        // Handle FILE only
        fd = open(argv[1], 0);
        if (fd < 0) {
            printf(2, "tail: cannot open file %s\n", argv[1]);
            exit();
        }
    } else if (argc == 1) {
        // Handle stdin
        fd = 0;
    } else {
        printf(2, "Usage: tail [-NUM | -n NUM] [file]\n");
        exit();
    }

    tail(fd, num_lines);

    if (fd > 0) close(fd);

    exit();
}
