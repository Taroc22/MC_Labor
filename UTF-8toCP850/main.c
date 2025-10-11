/*
    @desc: converts all string literals ("" & '') from UTF-8 to CP850
    @author: Amir Tannouri | 2025
	@cmd: gcc main.c -o main.exe && main.exe input.c out.c
	
	@info: escape sequences are not fully implemented yet (e.g. \x43 too)
	//Festlegung: Alle Zeichen innerhalb von "" werden als \xYY dargestellt
	//Escape Sequences werden unverändert eingefügt (Feststellen der Länge der Sequenz)
	//'' enthalten maxmial 1byte (auch escape Sequence <= 1byte möglich)
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "CP850.h"

#define FALLBACK '?'	//replaces invalid characters; must not be A-F


//replaces valid utf-8 characters that are non-existent in CP850
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


//writes replacement characters as valid sequences
void write_escaped_for_c_literal(FILE *f, const char *s) {
    if (!s) {
        fputc(FALLBACK, f);
        return;
    }
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        unsigned char ch = *p;
        switch (ch) {
            case '"':
				fprintf(f, "\\x%02X", 0x5c); // '\'
				fprintf(f, "\\x%02X", 0x22); // '"'
				break;
			case '\\': 
				fprintf(f, "\\x%02X", 0x5c); // '\'
				fprintf(f, "\\x%02X", 0x5c); // '\'
				break;
            default:
				fprintf(f, "\\x%02X", ch);
        }
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

//returns 1 if mapping exists, 0 if not
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
                fputc(c, fout); // Escaped char direkt schreiben
                escape = false;
                continue;
            }

            if (c == '\\') {
                escape = true;
                fputc(c, fout);
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
