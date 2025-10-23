/*
    @desc: converts all literals ("" & '') from UTF-8 to CP850 for C programs
    @author: Amir Tannouri | 2025
	@cmd: gcc main.c -o main.exe && main.exe input.c out.c
	
	@info: all valid C escape sequences (\a\b\e\f\n\r\t\v\\\'\"\?) stay untouched and are valid in CP850
	@info: \nnn, \uhhhh and \Uhhhhhhhh stay untouched
	@info: \xhh… stays untouched; digit count is limited, contrary to the c standard, to 1 byte to prevent char overflow
	@info: ignores all literals inside of comments
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "CP850.h"

// replaces invalid characters; must not be A-F
#define FALLBACK '?'	

// returns replacement for valid utf-8 characters that are non-existent in CP850
static const char* fallback_string(uint32_t cp) {
    switch (cp) {
        case 0x20AC: return "EUR";    // € -> "EUR"
        case 0x201C: return "\"";     // “ -> "
        case 0x201D: return "\"";     // ” -> "
        case 0x2018: return "'";      // ‘ -> '
        case 0x2019: return "'";      // ’ -> '
        case 0x2026: return "...";    // … -> ...
        case 0x2013: return "-";      // – -> -
        case 0x2014: return "--";     // — -> --
        default:     return NULL;
    }
}


// writes replacement characters as valid sequences
static void write_escaped_for_c_literal(FILE *f, const char *s) {
    if (!s) {
        fputc(FALLBACK, f);
        return;
    }
    while (*s) {
        switch ((unsigned char)*s) {
            case '"':
				fprintf(f, "\\x%02X", 0x5c); // '\'
				fprintf(f, "\\x%02X", 0x22); // '"'
				break;
			case '\\': 
				fprintf(f, "\\x%02X", 0x5c); // '\'
				fprintf(f, "\\x%02X", 0x5c); // '\'
				break;
            default:
				fprintf(f, "\\x%02X", *s);
        }
		s++;
    }
}


static uint32_t utf8_to_unicode(const char *utf8, int bytescount) {
    uint32_t codepoint = 0;

    if (bytescount == 1) {
        // 1-Byte ASCII
        codepoint = (uint8_t)utf8[0];
    } else if (bytescount == 2) {
        // 110xxxxx 10xxxxxx
        codepoint = ((utf8[0] & 0x1F) << 6) |
                     (utf8[1] & 0x3F);
    } else if (bytescount == 3) {
        // 1110xxxx 10xxxxxx 10xxxxxx
        codepoint = ((utf8[0] & 0x0F) << 12) |
                    ((utf8[1] & 0x3F) << 6) |
                     (utf8[2] & 0x3F);
    } else if (bytescount == 4) {
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        codepoint = ((utf8[0] & 0x07) << 18) |
                    ((utf8[1] & 0x3F) << 12) |
                    ((utf8[2] & 0x3F) << 6) |
                     (utf8[3] & 0x3F);
    } else {
        // invalid byte count; should not be possible to occure
        return (uint32_t)FALLBACK;
    }

    return codepoint;
}

// returns 1 if mapping exists, 0 if not
static int unicode_to_cp850(uint32_t codepoint, uint8_t *out_cp) {
    if (codepoint < 128) { //ASCII
        *out_cp = (uint8_t)codepoint;
        return 1;
    }
    for (int i = 0; cp850_map[i].unicode; ++i) {
        if (cp850_map[i].unicode == codepoint) {
            *out_cp = cp850_map[i].cp850;
            return 1;
        }
    }
    return 0;
}


static inline bool is_hex(int c) {
    return isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f');
}


// reads 'count' hex chars
static void read_fixed_hex(FILE* fin, FILE* fout, int c, int count) {
	char hex[count];
    for (int i = 0; i < count; ++i) {
        int h = fgetc(fin);
        if (h == EOF || !is_hex(h)) {
            if (h != EOF)
                ungetc(h, fin);
			fputc(FALLBACK, fout);
            return;
        }
		hex[i] = h;
    }
	fputc('\\', fout);
	fputc(c, fout);
	for (int i = 0; i < count; i++) {
		fputc(hex[i], fout);
    }
}


// reads 1-3 octal chars
static void read_octal(FILE* fin, FILE* fout, int c) {
    int count = 1;
	char oct[3];
	oct[0] = c;
    while (count < 3) {
        int d = fgetc(fin);
        if (d == EOF || d < '0' || d > '7') {
            if (d != EOF)
                ungetc(d, fin);
			fputc(FALLBACK, fout);
            return;
        }
		oct[count] = d;
        count++;
    }
	fputc('\\', fout);
	for (int i = 0; i < 3; i++) {
		fputc(oct[i], fout);
    }
}


// reads any number of consecutive hex chars
static void read_hex(FILE* fin, FILE* fout) {
    char hex[8];
    int count = 0;
    for (int i = 0; i < 8; ++i) {
        int d = fgetc(fin);
        if (d == EOF || !is_hex(d)) {
            if (d != EOF)
                ungetc(d, fin);
            break;
        }
        hex[count] = d;
		count++;
    }
    if (count == 0) {
        fputc(FALLBACK, fout);
        return;
    }
    fputc('\\', fout);
    fputc('x', fout);
    for (int i = 0; i < count; ++i) {
        fputc(hex[i], fout);
    }
}


// extract UTF-8 character
static int handle_char(FILE *fin, FILE *fout, char c, bool *valid, bool in_char){

	unsigned char first = (unsigned char)c;
	int expected_bytes = 0;

	if ((first & 0x80) == 0) {
		expected_bytes = 1; // ASCII
	} else if ((first & 0xE0) == 0xC0) {
		expected_bytes = 2;
	} else if ((first & 0xF0) == 0xE0) {
		expected_bytes = 3;
	} else if ((first & 0xF8) == 0xF0) {
		expected_bytes = 4;
	} else {
		fprintf(stderr, "Invalid UTF-8 start byte: 0x%x\n", first);
		fputc(FALLBACK, fout);
		return 0;
	}

	char utf8_bytes[4];
	utf8_bytes[0] = first;

	for (int i = 1; i < expected_bytes; i++) {
		int next = fgetc(fin);
		if (next == EOF) {
			*valid = false;
			return 1;
		}
		if ((next & 0xC0) != 0x80) {
			fprintf(stderr, "Invalid UTF-8 continuation byte: 0x%x\n", next);
			*valid = false;
			return 1;
		}
		utf8_bytes[i] = (char)next;
		*valid = true;
	}
	
	
	if (*valid) {
		uint32_t unicode = utf8_to_unicode(utf8_bytes, expected_bytes);
		uint8_t cp;
		if (unicode_to_cp850(unicode, &cp)) {
			fprintf(fout, "\\x%02X", cp);
		} else {
			const char *rep = fallback_string(unicode);
			if (in_char) {
				// max 1 char
				if (rep && rep[0] && rep[1] == '\0' && rep[0] != '\'' && rep[0] != '\\' && isprint((unsigned char)rep[0])) {
					fputc(rep[0], fout);
				} else {
					fputc(FALLBACK, fout);
				}
			} else {// in string
				if (rep) {
					write_escaped_for_c_literal(fout, rep);
				} else {
					fputc(FALLBACK, fout);
				}
			}
		}
	} else {
		fputc(FALLBACK, fout);
		fprintf(stderr, "Invalid UTF-8 character\n");
	}
	return 0;
}

static void handle_escape(int c, FILE* fout, FILE* fin) {
    switch (c) {
        case 'a': case 'b': case 'e': case 'f':
        case 'n': case 'r': case '"': case '?':
        case 't': case 'v': case '\\': case '\'':
            fputc('\\', fout);
            fputc(c, fout);
            break;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            // \nnn (octal)
			read_octal(fin, fout, c);
            break;

        case 'x':
            // \xhh...
			read_hex(fin, fout);
            break;

        case 'u':
            // \uhhhh
            read_fixed_hex(fin, fout, c, 4);
            break;

        case 'U':
            // \Uhhhhhhhh
			read_fixed_hex(fin, fout, c, 8);
            break;

        default:
            fputc(FALLBACK, fout); // dont write escape sequence
            break;
    }
}


static void convert(const char* input_path, const char* output_path) {
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
	bool valid = true;
	bool in_line_comment = false;
	bool in_block_comment = false;
	int prev = 0;

    while ((c = fgetc(fin)) != EOF) {
		if (in_line_comment) {
            fputc(c, fout);
            if (c == '\n') {
                in_line_comment = false;
            }
            continue;
        }
		
		if (in_block_comment) {
			fputc(c, fout);
			if (prev == '*' && c == '/') {
				in_block_comment = false;
				prev = 0;
				continue;
			}
			prev = c;
			continue;
		}
		
		
        if (in_string || in_char) {
            char quote = in_string ? '"' : '\'';

            if (escape) {
				handle_escape(c, fout, fin);
                escape = false;
                continue;
            }

            if (c == '\\') {
                escape = true;
                continue;
            }

            if (c == quote) {
                fputc(quote, fout);
                in_string = false;
                in_char = false;
                continue;
            }
			
			int tmp = handle_char(fin, fout, c, &valid, in_char);
			if (tmp == 0)
				continue;
			else if (tmp == 1)
				break;
			
        } else {
            if (prev == '/' && c == '/') {
				in_line_comment = true;
				fputc('/', fout);
				fputc('/', fout);
				prev = 0;
				continue;
			} else if (prev == '/' && c == '*') {
				in_block_comment = true;
				fputc('/', fout);
				fputc('*', fout);
				prev = 0;
				continue;
			} else if (prev == '/') {	//handles standalone '/'; e.g. division
				fputc('/', fout);
			}
			
			
            if (c == '"') {
                in_string = true;
                fputc('"', fout);
				prev = 0;
                continue;
            } else if (c == '\'') {
                in_char = true;
                fputc('\'', fout);
				prev = 0;
                continue;
            }
        }
		
        if (c != '/') {
            fputc(c, fout);
            prev = c;
        } else {
            prev = '/';
        }
    }
	
	if (prev == '/') fputc('/', fout);
    fclose(fin);
    fclose(fout);
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.c> <output.c>\n", argv[0]);
        return 1;
    }

    convert(argv[1], argv[2]);
    printf("File processed: %s -> %s\n", argv[1], argv[2]);
    return 0;
}
