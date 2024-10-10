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
    int num_parts;
    char parts[MAX_PARTS][50];
} Concatenate;

typedef struct {
    char name[50];
    char from[50];
} StderrNode;

Node nodes[MAX_NODES];
PipeNode pipes[MAX_NODES];
Concatenate concatenates[MAX_NODES];
StderrNode stderrs[MAX_NODES];
int node_count = 0, pipe_count = 0, concatenate_count = 0, stderr_count = 0;

void execute_command(const char* command) {
    if (strlen(command) > 0) {
        system(command);
    }
}

char* get_node_command(const char* node_name) {
    for (int i = 0; i < node_count; i++) {
        if (strcmp(nodes[i].name, node_name) == 0) {
            return nodes[i].command;
        }
    }

    for (int i = 0; i < stderr_count; i++) {
        if (strcmp(stderrs[i].name, node_name) == 0) {
            for (int j = 0; j < node_count; j++) {
                if (strcmp(nodes[j].name, stderrs[i].from) == 0) {
                    static char stderr_command[100];
                    sprintf(stderr_command, "%s 2>&1", nodes[j].command);
                    return stderr_command;
                }
            }
        }
    }

    return NULL;
}

void build_concatenate_command(const char* concat_name, char* result) {
    for (int i = 0; i < concatenate_count; i++) {
        if (strcmp(concatenates[i].name, concat_name) == 0) {
            strcat(result, "(");  // Add opening parenthesis for grouping
            for (int j = 0; j < concatenates[i].num_parts; j++) {
                char* cmd = get_node_command(concatenates[i].parts[j]);

                if (cmd) {
                    if (j > 0 && strlen(result) > 1) strcat(result, "; ");
                    strcat(result, cmd);
                } else {
                    for (int k = 0; k < pipe_count; k++) {
                        if (strcmp(pipes[k].name, concatenates[i].parts[j]) == 0) {
                            char pipe_cmd[1000] = "";
                            char* from_cmd = get_node_command(pipes[k].from);
                            char* to_cmd = get_node_command(pipes[k].to);
                            if (from_cmd && to_cmd) {
                                sprintf(pipe_cmd, "%s | %s", from_cmd, to_cmd);
                                if (j > 0 && strlen(result) > 1) strcat(result, "; ");
                                strcat(result, pipe_cmd);
                            }
                            break;
                        }
                    }
                }
            }
            strcat(result, ")");  // Add closing parenthesis for grouping
            return;
        }
    }
}

void execute_node(const char* node_name) {
    char* cmd = get_node_command(node_name);
    if (cmd) {
        execute_command(cmd);
        return;
    }

    for (int i = 0; i < concatenate_count; i++) {
        if (strcmp(concatenates[i].name, node_name) == 0) {
            char concat_cmd[1000] = "";
            build_concatenate_command(node_name, concat_cmd);
            if (strlen(concat_cmd) > 0) {
                execute_command(concat_cmd);
            }
            return;
        }
    }

    for (int i = 0; i < pipe_count; i++) {
        if (strcmp(pipes[i].name, node_name) == 0) {
            char pipe_cmd[1000] = "";
            char* from_cmd = get_node_command(pipes[i].from);
            if (!from_cmd) {
                build_concatenate_command(pipes[i].from, pipe_cmd);
            } else {
                strcpy(pipe_cmd, from_cmd);
            }

            if (strlen(pipe_cmd) == 0) {
                fprintf(stderr, "Error: Empty command before pipe '%s'\n", node_name);
                return;
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

    fprintf(stderr, "Error: Node '%s' not found\n", node_name);
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
        } else if (strcmp(token, "stderr") == 0) {
            StderrNode stderr_node;
            strcpy(stderr_node.name, strtok(NULL, "\n"));
            fgets(line, sizeof(line), file);
            sscanf(line, "from=%s", stderr_node.from);
            stderrs[stderr_count++] = stderr_node;
        }
    }

    fclose(file);
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