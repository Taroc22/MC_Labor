#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//gcc main.c -o main.exe && main.exe input.c out.c "REPLACEMENT"

void replace(const char* input_path, const char* output_path, const char* replacement) {
    FILE* fin = fopen(input_path, "r");
    if (!fin) {
        perror("Error while opening input file");
        exit(1);
    }

    FILE* fout = fopen(output_path, "w");
    if (!fout) {
        perror("Error while opening output file");
        fclose(fin);
        exit(1);
    }

    int c;
    bool in_string = false;
    bool in_char = false;
    bool escape = false;

    while ((c = fgetc(fin)) != EOF) {
        if (in_string) {
            if (escape) {
                escape = false;
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == '"') {
                in_string = false;
            }
            continue;
        } else if (in_char) {
            if (escape) {
                escape = false;
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == '\'') {
                in_char = false;
            }
            continue;
        } else {
            if (c == '"') {
                in_string = true;
                fputs(replacement, fout);
                continue;
            } else if (c == '\'') {
                in_char = true;
                fputs(replacement, fout);
                continue;
            }
        }
        fputc(c, fout);
    }

    fclose(fin);
    fclose(fout);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input.c> <output.c> <replacement>\n", argv[0]);
        return 1;
    }

    replace(argv[1], argv[2], argv[3]);
    printf("File processed: %s -> %s\n", argv[1], argv[2]);
    return 0;
}
