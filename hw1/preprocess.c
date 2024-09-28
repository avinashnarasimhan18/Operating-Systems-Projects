#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_VARS 10
#define MAX_VAR_LEN 128 
#define MAX_VAL_LEN 128
#define MAX_LINE_LEN 512

struct definition {
    char var[MAX_VAR_LEN];
    char val[MAX_VAL_LEN];
};

// Custom version of strncmp for xv6 
int my_strncmp(char *str1, char *str2, int n) {
    int i;
    for (i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        if (str1[i] == '\0' || str2[i] == '\0') {
            break;
        }
    }
    return 0;
}

// Function to check if a character is a valid identifier
int is_valid_identifier_char(char c, int is_start) {
    if (is_start) {
        return (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
    }
    return (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
}

// Function to check if a string starts with a variable name that is a valid C identifier
int starts_with_var(char *str, char *var, char *start_of_str) {
    int len = strlen(var);

    // Check if the variable is preceded by a valid boundary (start of string or non-alphanumeric character)
    if (str == start_of_str || !is_valid_identifier_char(str[-1], 0)) {
        // Check if the variable name matches and is followed by a valid boundary
        if (my_strncmp(str, var, len) == 0) {
            if (str[len] == '\0' || str[len] == ' ' || str[len] == '\t' || str[len] == '\n' ||
                str[len] == '.' || str[len] == ',' || str[len] == '!' || str[len] == '?' || 
                !is_valid_identifier_char(str[len], 0)) {
                return 1;
            }
        }
    }
    return 0;
}

// Custom version of strncpy for xv6 
void my_strncpy(char *dest, char *src, int n) {
    int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
}

// Function to check if a line is a #define directive
int is_define_line(char *line) {
    // Skip leading whitespace
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    // Check if the line starts with "#define"
    return (my_strncmp(line, "#define", 7) == 0);
}

// Function to substitute variables in a line with their values 
void substitute_variables(char *line, struct definition *defs, int num_defs) {
    // If the line starts with #define, ignore it completely
    if (is_define_line(line)) {
        return;  // Skip the line without printing or processing
    }

    char result[MAX_LINE_LEN];
    int result_index, i, j, k, matched;
    int original_non_empty = 0; // Flag to track if the line was originally non-empty

    // We loop until no further substitutions are made in a pass
    int substitutions_made;

    do {
        substitutions_made = 0;  // Track if any substitution happens in this pass
        result_index = 0;
        i = 0;

        while (line[i] != '\0') {
            matched = 0;
            for (j = 0; j < num_defs; j++) {
                int len = strlen(defs[j].var);
                // Use starts_with_var to ensure we match only valid C identifier variable names
                if (starts_with_var(&line[i], defs[j].var, line)) {
                    for (k = 0; defs[j].val[k] != '\0'; k++) {
                        result[result_index++] = defs[j].val[k];
                    }
                    i += len; 
                    matched = 1;
                    substitutions_made = 1;  // Mark that a substitution was made
                    original_non_empty = 1;  // This means the line had content before substitution
                    break;
                }
            }
            if (!matched) {
                if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
                    original_non_empty = 1;  // Mark if the line was non-empty before substitution
                }
                result[result_index++] = line[i++];
            }
        }

        result[result_index] = '\0';  // Null-terminate the result string

        // Copy result back into line to allow further substitutions
        my_strncpy(line, result, MAX_LINE_LEN);

    } while (substitutions_made);  // Keep substituting until no more changes are made

    // Skip printing if the line is completely empty after substitution
    if (original_non_empty || result_index > 0) {
        printf(1, "%s\n", result);  // Print each substituted line if it originally had content or is not empty
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(1, "Usage: preprocess <input_file> -D<var1>=<val1> -D<var2>=<val2> ...\n");
        exit();
    }

    // Open the input file
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf(1, "Error: Cannot open file %s\n", argv[1]);
        exit();
    }

    struct definition defs[MAX_VARS];
    int num_defs = 0;
    int i;

    // Parse the command line definitions (-Dvar=val)
    for (i = 2; i < argc; i++) {
        if (my_strncmp(argv[i], "-D", 2) == 0) {
            char *def = argv[i] + 2; 
            char *equal_sign = strchr(def, '=');
            int var_len;

            if (equal_sign != 0) {
                // If '=' is found, parse normally
                var_len = equal_sign - def;
                if (var_len < MAX_VAR_LEN && strlen(equal_sign + 1) < MAX_VAL_LEN) {
                    my_strncpy(defs[num_defs].var, def, var_len);
                    defs[num_defs].var[var_len] = '\0';
                    my_strncpy(defs[num_defs].val, equal_sign + 1, MAX_VAL_LEN);
                    num_defs++;
                } else {
                    printf(1, "Error: Variable or value too long\n");
                    exit();
                }
            } else {
                // If '=' is not found, set the value to "1"
                var_len = strlen(def);
                if (var_len < MAX_VAR_LEN) {
                    my_strncpy(defs[num_defs].var, def, var_len);
                    defs[num_defs].var[var_len] = '\0';
                    my_strncpy(defs[num_defs].val, "1", MAX_VAL_LEN);
                    num_defs++;
                } else {
                    printf(1, "Error: Variable too long\n");
                    exit();
                }
            }
        }
    }

    if (num_defs == 0) {
        printf(1, "Error: No valid definitions provided\n");
        exit();
    }

    // Read the input file line by line and substitute variables
    char line[MAX_LINE_LEN];
    char c;
    int line_index = 0;
    
    while (read(fd, &c, 1) > 0) {
        if (c == '\n' || line_index == MAX_LINE_LEN - 1) {
            line[line_index] = '\0';  // Null-terminate the line
            substitute_variables(line, defs, num_defs);
            line_index = 0;  // Reset for the next line
        } else {
            line[line_index++] = c;
        }
    }

    // Handle the last line if there's no newline at the end
    if (line_index > 0) {
        line[line_index] = '\0';
        substitute_variables(line, defs, num_defs);
    }

    close(fd);
    exit();
}

