/*
    @desc: converts all string literals ("" & '') from UTF-8 to CP850
    @author: Amir Tannouri | 2025
	@cmd: gcc main.c -o main.exe && main.exe input.c out.c "REPLACEMENT"
	@info: escape sequences are not implemented yet 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "CP850.h"

#define FALLBACK '?'


void convert(const char* input_path, const char* output_path, const char* replacement) {
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
        if (in_string || in_char) {
            char quote = in_string ? '"' : '\'';

            if (c == quote) {
                fputc(quote, fout);
                in_string = false;
                in_char = false;
                continue;
            }
			
			// c checken und mappen
			
            fputs(replacement, fout);
            continue;

        } else {
            if (c == '"') {
                in_string = true;
                fputc('"', fout);
                continue;
            } else if (c == '\'') {
                in_char = true;
                fputc('\'', fout);
                continue;
            }
        }

        fputc(c, fout);
    }

    fclose(fin);
    fclose(fout);
}

uint8_t fallback(uint32_t cp){
	return FALLBACK;
}


//def utf8_to_unicode()
//return everything below 127 as it is, just as an uint32_t
//if unicode would be >1byte return 256 for fallback in unicode_to_cp850()

//first 128 characters in CP850 == ASCII => same unicode
uint8_t unicode_to_cp850(uint32_t codepoint) {
	if (codepoint > 255)
		return fallback(codepoint);
	if (codepoint < 128)
        return (uint8_t)codepoint;
    for (int i = 0; cp850_map[i].unicode; ++i)
        if (cp850_map[i].unicode == codepoint)
            return cp850_map[i].cp850;
    return fallback(codepoint);
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input.c> <output.c> <replacement>\n", argv[0]);
        return 1;
    }

    convert(argv[1], argv[2], argv[3]);
    printf("File processed: %s -> %s\n", argv[1], argv[2]);
    return 0;
}
