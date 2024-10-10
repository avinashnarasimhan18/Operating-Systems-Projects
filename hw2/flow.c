#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_NODES 100
#define MAX_LINE_LENGTH 1000
#define MAX_PARTS 10

typedef struct {
    char name[50];
    char command[100];
} Node;

typedef struct {
    char name[50];
    char from[50];
    char to[50];
} PipeNode;

typedef struct {
    char name[50];
    char from[50];
} StderrNode;

typedef struct {
    char name[50];
    int num_parts;
    char parts[MAX_PARTS][50];
} Concatenate;

// Global Arrays to Store Nodes, Pipes, and Concatenates
Node nodes[MAX_NODES];
PipeNode pipes[MAX_NODES];
StderrNode stderr_nodes[MAX_NODES];
Concatenate concatenates[MAX_NODES];
int node_count = 0, pipe_count = 0, stderr_count = 0, concatenate_count = 0;

// Function Prototypes (Declarations)
void parse_command(const char* command, char** args);
void execute_command(const char* command);
void execute_piped_command(const char* command1, const char* command2);
void build_concatenate_command(const char* concat_name, char* result);
char* get_node_command(const char* node_name);
void execute_node(const char* node_name);
void parse_flow_file(const char* filename);
char* get_stderr_source_command(const char* stderr_name);
int is_concatenate(const char* name);

char* get_node_command(const char* node_name) {
    for (int i = 0; i < node_count; i++) {
        if (strcmp(nodes[i].name, node_name) == 0) {
            return nodes[i].command;
        }
    }
    return NULL;
}

char* get_stderr_source_command(const char* stderr_name) {
    for (int i = 0; i < stderr_count; i++) {
        if (strcmp(stderr_nodes[i].name, stderr_name) == 0) {
            return get_node_command(stderr_nodes[i].from);
        }
    }
    return NULL;
}

int is_concatenate(const char* name) {
    for (int i = 0; i < concatenate_count; i++) {
        if (strcmp(concatenates[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

void execute_command(const char* command) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("sh", "sh", "-c", command, NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork failed");
    }
}

void execute_piped_command(const char* command1, const char* command2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp("sh", "sh", "-c", command1, NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        execlp("sh", "sh", "-c", command2, NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void build_concatenate_command(const char* concat_name, char* result) {
    for (int i = 0; i < concatenate_count; i++) {
        if (strcmp(concatenates[i].name, concat_name) == 0) {
            strcat(result, "(");
            for (int j = 0; j < concatenates[i].num_parts; j++) {
                if (j > 0) strcat(result, " ; ");

                char* cmd = get_node_command(concatenates[i].parts[j]);
                if (cmd) {
                    strcat(result, cmd);
                    continue;
                }

                for (int k = 0; k < pipe_count; k++) {
                    if (strcmp(pipes[k].name, concatenates[i].parts[j]) == 0) {
                        char* from_cmd = get_node_command(pipes[k].from);
                        char* to_cmd = get_node_command(pipes[k].to);

                        if (!from_cmd) {
                            from_cmd = get_stderr_source_command(pipes[k].from);
                            if (from_cmd) {
                                char stderr_redirect_cmd[1000];
                                sprintf(stderr_redirect_cmd, "%s 2>&1", from_cmd);
                                from_cmd = stderr_redirect_cmd;
                            }
                        }

                        if (from_cmd && to_cmd) {
                            char pipe_cmd[1000];
                            sprintf(pipe_cmd, "%s | %s", from_cmd, to_cmd);
                            strcat(result, pipe_cmd);
                        }
                    }
                }
            }
            strcat(result, ")");
            return;
        }
    }
}

void parse_flow_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char* token = strtok(line, "=");
        if (token == NULL) continue;

        if (strcmp(token, "node") == 0) {
            Node node;
            strcpy(node.name, strtok(NULL, "\n"));
            fgets(line, sizeof(line), file);
            sscanf(line, "command=%[^\n]", node.command);
            nodes[node_count++] = node;
        } else if (strcmp(token, "pipe") == 0) {
            PipeNode pipe;
            strcpy(pipe.name, strtok(NULL, "\n"));
            fgets(line, sizeof(line), file);
            sscanf(line, "from=%s", pipe.from);
            fgets(line, sizeof(line), file);
            sscanf(line, "to=%s", pipe.to);
            pipes[pipe_count++] = pipe;
        } else if (strcmp(token, "stderr") == 0) {
            StderrNode stderr_node;
            strcpy(stderr_node.name, strtok(NULL, "\n"));
            fgets(line, sizeof(line), file);
            sscanf(line, "from=%s", stderr_node.from);
            stderr_nodes[stderr_count++] = stderr_node;
        } else if (strcmp(token, "concatenate") == 0) {
            Concatenate concat;
            strcpy(concat.name, strtok(NULL, "\n"));
            fgets(line, sizeof(line), file);
            sscanf(line, "parts=%d", &concat.num_parts);
            for (int i = 0; i < concat.num_parts; i++) {
                fgets(line, sizeof(line), file);
                sscanf(line, "part_%*d=%s", concat.parts[i]);
            }
            concatenates[concatenate_count++] = concat;
        }
    }

    fclose(file);
}

void execute_node(const char* node_name) {
    char* cmd = get_node_command(node_name);
    if (cmd) {
        execute_command(cmd);
        return;
    }

    for (int i = 0; i < pipe_count; i++) {
        if (strcmp(pipes[i].name, node_name) == 0) {
            char pipe_cmd[1000] = "";
            
            if (is_concatenate(pipes[i].from)) {
                build_concatenate_command(pipes[i].from, pipe_cmd);
            } else {
                char* from_cmd = get_node_command(pipes[i].from);
                if (!from_cmd) {
                    from_cmd = get_stderr_source_command(pipes[i].from);
                    if (from_cmd) {
                        sprintf(pipe_cmd, "%s 2>&1", from_cmd);
                    }
                } else {
                    strcpy(pipe_cmd, from_cmd);
                }
            }

            strcat(pipe_cmd, " | ");
            char* to_cmd = get_node_command(pipes[i].to);
            if (to_cmd) {
                strcat(pipe_cmd, to_cmd);
            } else {
                fprintf(stderr, "Error: 'to' node not found for pipe %s\n", node_name);
                return;
            }
            execute_command(pipe_cmd);
            return;
        }
    }

    for (int i = 0; i < concatenate_count; i++) {
        if (strcmp(concatenates[i].name, node_name) == 0) {
            char concat_cmd[1000] = "";
            build_concatenate_command(node_name, concat_cmd);
            execute_command(concat_cmd);
            return;
        }
    }

    fprintf(stderr, "Error: Node '%s' not found\n", node_name);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <flow_file> <action>\n", argv[0]);
        return 1;
    }

    parse_flow_file(argv[1]);
    execute_node(argv[2]);

    return 0;
}
