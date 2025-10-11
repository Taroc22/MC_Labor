/*
    @desc: converts all literals ("" & '') from UTF-8 to CP850 for C programs
    @author: Amir Tannouri | 2025
	@cmd: gcc main.c -o main.exe && main.exe input.c out.c
	
	@info: all valid C escape sequences (\a\b\e\f\n\r\t\v\\\'\"\?) stay untouched and are valid in CP850
	@info: \nnn, \uhhhh and \Uhhhhhhhh stay untouched
	@info: \xhh… not decided yet how to handle
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
const char* fallback_string(uint32_t cp) {
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
void write_escaped_for_c_literal(FILE *f, const char *s) {
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


uint32_t utf8_to_unicode(const char *utf8, int bytescount) {
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
int unicode_to_cp850(uint32_t codepoint, uint8_t *out_cp) {
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


void handle_octal(FILE *fin, FILE *fout, char c){
	int oct[3];
	oct[0] = c;
	int count = 1;

	for (; count < 3; count++) {
		int next = fgetc(fin);
		if (next >= '0' && next <= '7') {
			oct[count] = next;
		} else { // revert to old cursor pos
			for (int j = count - 1; j >= 0; j--) {
				ungetc(oct[j], fin);
			}
			ungetc(next, fin);
			break;
		}
	}

	if (count == 3) {
		for (int i = 0; i < 3; i++) {
			fprintf(fout, "\\x%02X", oct[i]);
		}
	}else{
		fputc(FALLBACK, fout);
	}
}


void handle_u(FILE *fin, FILE *fout, char c){
	int oct[4];
	oct[0] = c;
	int count = 1;

	for (; count < 4; count++) {
		int next = fgetc(fin);
		if ((next >= '0' && next <= '9') || (next >= 'A' && next <= 'F') || (next >= 'a' && next <= 'f')) {
			oct[count] = next;
		} else { // revert to old cursor pos
			for (int j = count - 1; j >= 0; j--) {
				ungetc(oct[j], fin);
			}
			ungetc(next, fin);
			break;
		}
	}

	if (count == 4) {
		fprintf(fout, "\\x%02X", 'u');
		for (int i = 0; i < 4; i++) {
			fprintf(fout, "\\x%02X", oct[i]);
		}
	}else{
		fputc(FALLBACK, fout);
	}
}

void handle_U(FILE *fin, FILE *fout, char c){
	int oct[8];
	oct[0] = c;
	int count = 1;

	for (; count < 8; count++) {
		int next = fgetc(fin);
		if ((next >= '0' && next <= '9') || (next >= 'A' && next <= 'F') || (next >= 'a' && next <= 'f')) {
			oct[count] = next;
		} else { // revert to old cursor pos
			for (int j = count - 1; j >= 0; j--) {
				ungetc(oct[j], fin);
			}
			ungetc(next, fin);
			break;
		}
	}

	if (count == 8) {
		fprintf(fout, "\\x%02X", 'U');
		for (int i = 0; i < 8; i++) {
			fprintf(fout, "\\x%02X", oct[i]);
		}
	}else{
		fputc(FALLBACK, fout);
	}
}


void convert(const char* input_path, const char* output_path) {
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

    while ((c = fgetc(fin)) != EOF) {
        if (in_string || in_char) {
            char quote = in_string ? '"' : '\'';

            if (escape) {
				// Komplette vorangestelle \ Sache noch falsch
				switch (c) {
					case '0': case '1': case '2': case '3':
					case '4': case '5': case '6': case '7':
						// \nnn
						handle_octal(fin, fout, c);
						break;
					case 'a': case 'b': case 'e': case 'f':
					case 'n': case 'r': case '"': case '?':
					case 't': case 'v': case '\\': case '\'':
						fprintf(fout, "\\x%02X", c); //am Besten als char nicht hex schreiben
						break;
					case 'u': 
						// \uhhhh
						c = fgetc(fin);
						handle_u(fin, fout, c); 
						break;
					case 'U': 
						// \Uhhhhhhhh
						c = fgetc(fin);
						handle_U(fin, fout, c);
						break;
					case 'x': 
						// \xhh…
						//fprintf(fout, "\\x%02X", c); 
						break;
					default: fputc(c, fout); fputc(FALLBACK, fout); break; //????
				}
				printf("Test");
				printf("C: %c", c);	//c scheint empty; debug in handle functions
                escape = false;
                continue;
            }

            if (c == '\\') {
                escape = true;
				fputc('\\', fout); //Hier auch evtl. Fehler
                continue;
            }

            if (c == quote) {
                fputc(quote, fout);
                in_string = false;
                in_char = false;
                continue;
            }

            // extract UTF-8 character
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
                continue;
            }

            char utf8_bytes[4];
            utf8_bytes[0] = first;

            for (int i = 1; i < expected_bytes; i++) {
				int next = fgetc(fin);
				if (next == EOF) {
					valid = false;
					break;
				}
				if ((next & 0xC0) != 0x80) {
					fprintf(stderr, "Invalid UTF-8 continuation byte: 0x%x\n", next);
					valid = false;
					break;
				}
				utf8_bytes[i] = (char)next;
				valid = true;
			}
			
			
			if (valid) {
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


int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.c> <output.c>\n", argv[0]);
        return 1;
    }

    convert(argv[1], argv[2]);
    printf("File processed: %s -> %s\n", argv[1], argv[2]);
    return 0;
}
