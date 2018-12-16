#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include "pidfile.c"

#include "../config.h"


//FIXME: Better way to distinguish if macro was set
//#if IGNORE_COMPLEX_SHORTCUTS==0
enum access_type_t { UNSET_TYPE, BUS, DEVICE, REGISTER };
enum access_mode_t { UNSET_MODE, WORD, BYTE, RWBYTE, WORD_PEC, BYTE_PEC, RWBYTE_PEC };
enum access_expect_t { UNSET_EXPECT, VALUE, NONE, FAIL };

typedef enum access_type_t  access_type;
typedef enum access_mode_t access_mode;
typedef enum access_expect_t access_expect;

typedef struct {
  int bus;
  char name[MAX_SHORTCUT_LENGTH];
  access_type type;
  access_mode mode;
  access_expect expect;
  int device_address;
  int register_address;
  int expect_value;
  char exec_before[MAX_SHELL_COMMAND_LENGHT];
  char exec_after[MAX_SHELL_COMMAND_LENGHT];
} shortcut_struct;

//#else
//typedef struct { int bus; char name[MAX_SHORTCUT_LENGTH]; } shortcut_struct;
//#endif

#ifdef HAVE_USER_SHORTCUTS_FILE
// File for hardcoded Shortcuts
#include "user_shortcuts.h"
#endif

#ifndef USER_SHORTCUTS_H
shortcut_struct static_shortcuts[] = { {} };
#endif

#if IGNORE_COMPLEX_SHORTCUTS==0
enum options_e { HAS_TYPE, HAS_MODE, HAS_EXPECT, HAS_DEVICE_ADDR, HAS_REGISTER_ADDR, HAS_EXEC_BEFORE, HAS_EXEC_AFTER };
#endif
unsigned char complex_shortcut_options = 0;

shortcut_struct *dynamic_shortcuts = NULL;
int dynamic_shortcuts_size = 0;

#define bool int
#define true 1
#define false 0

bool verbose = false;
bool quiet = false;

bool input_is_shortcut = true;

int _access_type = UNSET_TYPE;
int _access_mode = UNSET_MODE;
int _access_expect = UNSET_EXPECT;
int _device_address = -1;
int _register_address = -1;
int _expect_value = -1;
char _exec_before[MAX_SHELL_COMMAND_LENGHT] = "";
char _exec_after[MAX_SHELL_COMMAND_LENGHT] = "";

#define BOLD printf("\e[1m");
#define UNDERLINED printf("\e[4m");
#define ITALIC printf("\e[3m");
#define RESETTEXT printf("\e[0m");


void usage(char *progname) {

  // check for unicode support in terminal
  int i;
  char *str = getenv ("LANG");

  for (i = 0; str[i + 2] != 00; i++) {
    if ((str[i] == 'u' && str[i + 1] == 't' && str[i + 2] == 'f')
        || (str[i] == 'U' && str[i + 1] == 'T' && str[i + 2] == 'F')) {
      i = -1;
      break;
    }
  }
  // if i == -1, unicode output is supported

  printf("\n");
  printf("The \"i2cselect\" utility for ");
#if USE_TEXT_FORMATING==1
  ITALIC
  printf("Raspberry Pi");
  RESETTEXT
#else
  printf("Raspberry Pi");
#endif
  printf(" computers is intended for\n");

  if (i != -1) {
    printf("use in conjunction with a TCA9548* I2C-Multiplexer and the \"i2c-mux\"\n");
  } else {
    printf("in conjunction with a TCA9548* I\u00B2C-Multiplexer and the \"i2c-mux\"\n");
  }
  printf("Device-Tree Overlay as shipped with the Raspbian Linux Distribution\n");
  printf("(File: \'/boot/overlays/i2c-mux.dtbo\').\n");
  printf("\n");
  printf("\n");
#if USE_TEXT_FORMATING==1
  BOLD
  printf("\tUSAGE:\t  %s [-v|-q|-h] BUS\n\n", progname);
  RESETTEXT
#else
  printf("\tUSAGE:\t  %s [-v|-q|-h] BUS\n\n", progname);
#endif
  if (i != -1) {
    printf("\t      \t  where BUS is a valid I2C-device number\n");
  } else {
    printf("\t      \t  where BUS is a valid I\u00B2C-device number\n");
  }
  printf("\n");
  printf("\t      \t   -v (--verbose)  be more talkative\n");
  printf("\t      \t   -q (--quiet)    don't output any messages\n");
  printf("\t      \t   -h (--help)     display this help text\n");
  printf("\n");
#if USE_TEXT_FORMATING==1
  BOLD
  printf("WARNING!");
  RESETTEXT
#else
  printf("WARNING!");
#endif
  if (i != -1) {
    printf(" This program can confuse your I2C bus, cause data loss and worse!\n");
  } else {
    printf(" This program can confuse your I\u00B2C bus, cause data loss and worse!\n");
  }
  printf("         Only use it if you are absolutely certain about what you are doing!\n");
  printf("\n");
  printf("\n");
  if (i != -1) {
    printf("Licensed under the terms of the GPLv2 license by Tobias Vogel. (C) 2018.\n");
  } else {
    printf("Licensed under the terms of the GPLv2 license by Tobias Vogel. \u00A9 2018.\n");
  }
  printf("\n");
  printf("Please report bugs at ");
#if USE_TEXT_FORMATING==1
  UNDERLINED
  printf("https://github.com/tobiasvogel/i2cselect");
  RESETTEXT
#else
  printf("<https://github.com/tobiasvogel/i2cselect>");
#endif
  printf(".");
  printf("\n");
  printf("\n");
  printf("\n");
  printf(" * See http://www.ti.com/product/TCA9548A for the current Datasheet of the\n");
  if (i != -1) {
    printf("   Texas Instruments TCA9548 I2C-Multiplexer IC\n");
  } else {
    printf("   Texas Instruments TCA9548 I\u00B2C-Multiplexer IC\n");
  }
  printf("\n");
  clean_exit(127);
}

#if IGNORE_COMPLEX_SHORTCUTS==0
bool get_option(enum options_e option) {

// Since C does not allow for binary values, here is the
// bitwise options we are reading in:
//
//	HAS_TYPE 		= 0b0000001;
//	HAS_MODE 		= 0b0000010;
//	HAS_EXPECT 		= 0b0000100;
//	HAS_DEVICE_ADDR 	= 0b0001000;
//	HAS_REGISTER_ADDR 	= 0b0010000;
//	HAS_EXEC_BEFORE		= 0b0100000;
//	HAS_EXEC_AFTER		= 0b1000000;
  unsigned char bitread = 0;
  unsigned char bitmask = 0;
  switch (option) {
  case HAS_TYPE:
    bitmask = (1<<0);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>0);
    break;
  case HAS_MODE:
    bitmask = (1<<1);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>1);
    break;
  case HAS_EXPECT:
    bitmask = (1<<2);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>2);
    break;
  case HAS_DEVICE_ADDR:
    bitmask = (1<<3);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>3);
    break;
  case HAS_REGISTER_ADDR:
    bitmask = (1<<4);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>4);
    break;
  case HAS_EXEC_BEFORE:
    bitmask = (1<<5);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>5);
    break;
  case HAS_EXEC_AFTER:
    bitmask = (1<<6);
    bitread = ((complex_shortcut_options)&(bitmask));
    return (bitread>>6);
    break;
  default:
    return -1; // <-- should never be reached!!
  }
  return -1;
}

void set_option(enum options_e option) {
  unsigned char bitwrite = 0;
  switch (option) {
  case HAS_TYPE:
    bitwrite = (1<<0);
    break;
  case HAS_MODE:
    bitwrite = (1<<1);
    break;
  case HAS_EXPECT:
    bitwrite = (1<<2);
    break;
  case HAS_DEVICE_ADDR:
    bitwrite = (1<<3);
    break;
  case HAS_REGISTER_ADDR:
    bitwrite = (1<<4);
    break;
  case HAS_EXEC_BEFORE:
    bitwrite = (1<<5);
    break;
  case HAS_EXEC_AFTER:
    bitwrite = (1<<6);
    break;
  default:
    return; // <-- should never be reached!!
  }
  complex_shortcut_options = ((complex_shortcut_options)|(bitwrite));
}
#else // IGNORE_COMPLEX_SHORTCUTS==1
bool get_option(int option) {	// dummy function
  return 0;
}
#endif // IGNORE_COMPLEX_SHORTCUTS


int get_dynamic_shortcuts_size() {
  return dynamic_shortcuts_size;
}

void init_dynamic_shortcuts() {
  dynamic_shortcuts = (shortcut_struct*) malloc(sizeof(shortcut_struct));
  dynamic_shortcuts_size = 1;
}

void grow_dynamic_shortcuts() {
  shortcut_struct *_dynamic_shortcuts = (shortcut_struct*) malloc(sizeof(shortcut_struct)*(dynamic_shortcuts_size+1));
  for (int i=0; i < dynamic_shortcuts_size; ++i) {
    (_dynamic_shortcuts+i)->bus = (dynamic_shortcuts+i)->bus;
    strncpy(&(_dynamic_shortcuts+i)->name, (dynamic_shortcuts+i)->name, strlen((dynamic_shortcuts+i)->name));
    (_dynamic_shortcuts+i)->type = (dynamic_shortcuts+i)->type;
    (_dynamic_shortcuts+i)->mode = (dynamic_shortcuts+i)->mode;
    (_dynamic_shortcuts+i)->expect = (dynamic_shortcuts+i)->expect;
    (_dynamic_shortcuts+i)->device_address = (dynamic_shortcuts+i)->device_address;
    (_dynamic_shortcuts+i)->register_address = (dynamic_shortcuts+i)->register_address;
    (_dynamic_shortcuts+i)->expect_value = (dynamic_shortcuts+i)->expect_value;
    strncpy(&(_dynamic_shortcuts+i)->exec_before, (dynamic_shortcuts+i)->exec_before, strlen((dynamic_shortcuts+i)->exec_before));
    strncpy(&(_dynamic_shortcuts+i)->exec_after, (dynamic_shortcuts+i)->exec_after, strlen((dynamic_shortcuts+i)->exec_after));
  }
  dynamic_shortcuts = _dynamic_shortcuts;
  dynamic_shortcuts_size++;
}

void insert_in_dynamic_shortcuts(int dyn_bus, char *dyn_name, access_type dyn_type, access_mode dyn_mode, access_expect dyn_expect, int dyn_device_address, int dyn_register_address, int dyn_expect_value, char *dyn_exec_before, char* dyn_exec_after) {
  if (dynamic_shortcuts_size < 1) {
    init_dynamic_shortcuts();
  } else {
    grow_dynamic_shortcuts();
  }
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->bus = dyn_bus;
  strncpy(&(dynamic_shortcuts+(dynamic_shortcuts_size-1))->name, dyn_name, strlen(dyn_name));
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->type = dyn_type;
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->mode = dyn_mode;
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->expect = dyn_expect;
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->device_address = dyn_device_address;
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->register_address = dyn_register_address;
  (dynamic_shortcuts+(dynamic_shortcuts_size-1))->expect_value = dyn_expect_value;
  strncpy(&(dynamic_shortcuts+(dynamic_shortcuts_size-1))->exec_before, dyn_exec_before, strlen(dyn_name));
  strncpy(&(dynamic_shortcuts+(dynamic_shortcuts_size-1))->exec_after, dyn_exec_after, strlen(dyn_name));
}

char *to_lowercase(char *str) {
  char temp[MAX_SHORTCUT_LENGTH];
  strcpy(temp, str);
  int i;
  for (i = 0; i < strlen(str); i++) {
    if (temp[i] > 64 && temp[i] < 91) {	// 65 = ASCII A, 90 = ASCII Z
      temp[i] = temp[i] + 32;
    } else {
      temp[i] = temp[i];
    }
  }
  i++;
  if (i < MAX_SHORTCUT_LENGTH) {
    temp[i] = '\0';
  } else {
    temp[MAX_SHORTCUT_LENGTH-1] = '\0';
  }
  char *returnstr = (char *)malloc(strlen(temp)+1);
  strcpy(returnstr, temp);
  return returnstr;
}

#if IGNORE_EXTERNAL_SHORTCUTS_FILE==0
void read_external_shortcuts(char *filename) {
  FILE *fp;
  char *line = NULL;
  size_t lineno = 0;
  size_t len = 0;
  ssize_t read;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    if (!quiet) {
      fprintf(stderr, "Error: File \"%s\" could not be opened!\n", filename);
    }
    return;
  }

  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "=====\n");
    fprintf(VERBOSE_OUT_STREAM, "Reading File \"%s\" for additional shortcuts...\n", filename);
  }


  while ((read = getline(&line, &len, fp)) != -1) {
    if (len < 2) {
      continue;
    }
    lineno++;
    char linestart[3];
    snprintf(linestart, 2, "%s", line);
    if (linestart[0] == 35 || linestart[0] == 59) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is a comment, skipping.\n", lineno);
      }
      continue;
    }
    if (linestart[0] != 123 || strrchr(line, '}') == NULL) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring.\n", lineno);
      }
      continue;
    }
    if (verbose) {
      fprintf(VERBOSE_OUT_STREAM, "Processing line %d.\n", lineno);
    }
    //finding string range
    char *p;
    int end = -1;
    p = strchr(line, '}');
    if (p != NULL) {
      end = p - line;
    } else {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring.\n", lineno);
      }
      continue;
    }
    char parsestr[end+1];
    sprintf(parsestr, "%.*s", (end-1), line+1);
    parsestr[end] = '\0';
    int commas = 0;
    for (int i=0; i < end; i++) {
      commas += (parsestr[i] == ',');
    }

    // now that we know the segment-count on the line in question, process is

    int tempbus = -1;
    char *tempname;

    bool is_twosegment_shortcut = false;

    access_type temptype = UNSET_TYPE;
    access_mode tempmode = UNSET_MODE;
    access_expect tempexpect = UNSET_EXPECT;
    int tempdevice_address = 0;
    int tempregister_address = 0;
    int tempexpect_value = 0;
    char *tempexec_before = NULL;
    char *tempexec_after = NULL;

    if (commas == 0) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring.\n", lineno);
      }
      continue;
    }


    if (commas == 1) {
      //handle special short form where only a integer and a string are given (bus, name)
      if (parsestr[0] > 47 && parsestr[0] < 58) { // first char is numeric
        int sscancount = 0;
        char scanstr[MAX_SHORTCUT_LENGTH];
        sscancount = sscanf(parsestr, "%i%*[,]%*[\"']%[^\"']s", &tempbus, &scanstr);
        // copy back name with proper string termination
        if (strlen(scanstr) > 0) {
          tempname = (char*)malloc(strlen(scanstr)+1);
          scanstr[strlen(scanstr)] = '\0';
          strncpy(tempname, scanstr, strlen(scanstr)+1);
        }
        if (verbose) {
          fprintf(VERBOSE_OUT_STREAM, " -> Line in short format detected!\n");
          fprintf(VERBOSE_OUT_STREAM, " -> BUS: %d, NAME: %s\n", tempbus, tempname);
        }
        is_twosegment_shortcut = true;
      }
    }

    if (is_twosegment_shortcut == false) {
      char **segments = 0;
      segments = malloc(sizeof(char *) * (commas+1));
      size_t idx = 0;
      char *token = strtok(parsestr, ",");
      while(token) {
        if (idx <= commas) {
          *(segments + idx) = strdup(token);
          token = strtok(0, ",");
          idx++;
        } else {
          break;
        }
      }
      *(segments + idx) = 0;


      for (int i=0; i < (commas+1); i++) {
        char testkey[18];
        int sscancount = 0;
        strncpy(testkey, *(segments+i),	1);
        testkey[1] = '\0';
        if (testkey[0] == 'b' || testkey[0] == 'B') {	// BUS
          strncpy(testkey, *(segments+i), 4);
          testkey[4] = '\0';
          if (strcmp(to_lowercase(testkey), "bus=") != 0) {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          } else {
            sscancount = sscanf(*(segments+i), "%*3c%*[=]%i", &tempbus);
            if (sscancount != 1) {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> BUS : %d\n", tempbus);
              }
            }
          }
        } else if (testkey[0] == 'd' || testkey[0] == 'D') { // DEVICE_ADDR
          strncpy(testkey, *(segments+i), 12);
          testkey[12] = '\0';
          if (strcmp(to_lowercase(testkey), "device_addr=") != 0) {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          } else {
            sscancount = sscanf(*(segments+i), "%*11c%*[=]%x", &tempdevice_address);
            if (sscancount != 1) {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> DEVICE-ADDRESS : 0x%02X\n", tempdevice_address);
              }
            }
          }
        } else if (testkey[0] == 'e' || testkey[0] == 'E') { // EXPECT, EXPECT_VALUE, EXEC_BEFORE, EXEC_AFTER
          strncpy(testkey, *(segments+i), 7);
          testkey[7] = '\0';
          if (strcmp(to_lowercase(testkey), "expect=") == 0) {
            if (strcmp(to_lowercase(*(segments+i)), "expect=unset_expect") == 0) {
              tempexpect = UNSET_EXPECT;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> EXPECTED OPERATION RESULT: Not set (default).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "expect=value") == 0) {
              tempexpect = VALUE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> EXPECTED OPERATION RESULT: Specified Value.\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "expect=none") == 0) {
              tempexpect = NONE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> EXPECTED OPERATION RESULT: None or  not relevant (default).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "expect=fail") == 0) {
              tempexpect = FAIL;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> EXPECTED OPERATION RESULT: Read-Error or errornous Reading.\n");
              }
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            }
          } else if (strcmp(to_lowercase(testkey), "expect_") == 0) {
            sscancount = sscanf(*(segments+i), "%*12c%*[=]%*[0]%*[xX]%x", &tempexpect_value);
            if (sscancount != 1) {
              sscancount = sscanf(*(segments+i), "%*12c%*[=]%i", &tempexpect_value);
            }
            if (sscancount != 1) {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> EXPECTED READ VALUE: %d\n", tempexpect_value);
              }
            }
          } else if (strcmp(to_lowercase(testkey), "exec_be") == 0) {
            // get delimiter first
            strncpy(testkey, *(segments+i), 13);
            testkey[13] = '\0';
            char scanstr[MAX_SHELL_COMMAND_LENGHT+14];
            if (testkey[12] == 34) {
              sscancount = sscanf(*(segments+i), "%*11c%*[=]%*[\"]%[^\"]s", &scanstr);
              if (sscancount != 1) {
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
                }
                continue;
              } else {
                if (strlen(scanstr) > 0) {
                  // copy back name with proper string termination
                  tempexec_before = (char*)malloc(strlen(scanstr)+1);
                  scanstr[strlen(scanstr)] = '\0';
                  strncpy(tempexec_before, scanstr, strlen(scanstr)+1);
                }
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, " -> EXECUTE BEFORE: ( %s )\n", tempexec_before);
                }
              }
            } else if (testkey[12] == 39) {

              sscancount = sscanf(*(segments+i), "%*11c%*[=]%*[']%[^']s", &scanstr);
              if (sscancount != 1) {
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
                }
                continue;
              } else {
                if (strlen(scanstr) > 0) {
                  // copy back name with proper string termination
                  tempexec_before = (char*)malloc(strlen(scanstr)+1);
                  scanstr[strlen(scanstr)] = '\0';
                  strncpy(tempexec_before, scanstr, strlen(scanstr)+1);
                }
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, " -> EXECUTE BEFORE: ( %s )\n", tempexec_before);
                }
              }
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, Commands must be enclosed in either single (' ') or double (\" \") quotes.\n", *(segments+i));
              }
              continue;
            }
          } else if (strcmp(to_lowercase(testkey), "exec_af") == 0) {
            // get delimiter first
            strncpy(testkey, *(segments+i), 12);
            testkey[12] = '\0';
            char scanstr[MAX_SHELL_COMMAND_LENGHT+13];
            if (testkey[11] == 34) {

              sscancount = sscanf(*(segments+i), "%*10c%*[=]%*[\"]%[^\"]s", &scanstr);
              if (sscancount != 1) {
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
                }
                continue;
              } else {
                if (strlen(scanstr) > 0) {
                  // copy back name with proper string termination
                  tempexec_after = (char*)malloc(strlen(scanstr)+1);
                  scanstr[strlen(scanstr)] = '\0';
                  strncpy(tempexec_after, scanstr, strlen(scanstr)+1);
                }
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, " -> EXECUTE AFTER: ( %s )\n", tempexec_after);
                }
              }
            } else if (testkey[11] == 39) {

              sscancount = sscanf(*(segments+i), "%*10c%*[=]%*[']%[^']s", &scanstr);
              if (sscancount != 1) {
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
                }
                continue;
              } else {
                if (strlen(scanstr) > 0) {
                  // copy back name with proper string termination
                  tempexec_after = (char*)malloc(strlen(scanstr)+1);
                  scanstr[strlen(scanstr)] = '\0';
                  strncpy(tempexec_after, scanstr, strlen(scanstr)+1);
                }
                if (verbose) {
                  fprintf(VERBOSE_OUT_STREAM, " -> EXECUTE AFTER: ( %s )\n", tempexec_after);
                }
              }
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, Commands must be enclosed in either single (' ') or double (\" \") quotes.\n", *(segments+i));
              }
              continue;
            }
          } else {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          }

        } else if (testkey[0] == 'm' || testkey[0] == 'M') { // MODE
          strncpy(testkey, *(segments+i), 5);
          testkey[5] = '\0';
          if (strcmp(to_lowercase(testkey), "mode=") != 0) {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          } else {
            if (strcmp(to_lowercase(*(segments+i)), "mode=unset_mode") == 0) {
              tempmode = UNSET_MODE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Not set (default).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "mode=word") == 0) {
              tempmode = WORD;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Reading DWord.\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "mode=byte") == 0) {
              tempmode = BYTE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Single Byte Read (default Mode).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "mode=rwbyte") == 0) {
              tempmode = RWBYTE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Read/Write Access to read Byte value.\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "mode=wordpec") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=wordp") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=word_p") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=word_pec") == 0) {
              tempmode = WORD_PEC;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Reading DWord with Packet Error Checking (PEC).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "mode=bytepec") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=bytep") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=byte_p") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=byte_pec") == 0) {
              tempmode = BYTE_PEC;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Single Byte Read with Packet Error Checking (PEC).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "mode=rwbytepec") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=rwbytep") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=rwbyte_p") == 0 ||
                       strcmp(to_lowercase(*(segments+i)), "mode=rwbyte_pec") == 0) {
              tempmode = RWBYTE_PEC;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> ACCESS MODE: Read/Write Access to read Byte value with Packet Error Checking (PEC).\n");
              }
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            }
          }
        } else if (testkey[0] == 'n' || testkey[0] == 'N') { // NAME
          strncpy(testkey, *(segments+i), 5);
          testkey[5] = '\0';
          if (strcmp(to_lowercase(testkey), "name=") != 0) {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          } else {
            char scanstr[MAX_SHORTCUT_LENGTH];
            sscancount = sscanf(*(segments+i), "%*4c%*[=]%*[\"']%[^\"']s", &scanstr);
            if (sscancount != 1) {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            } else {
              if (strlen(scanstr) > 0) {
                // copy back name with proper string termination
                tempname = (char*)malloc(strlen(scanstr)+1);
                scanstr[strlen(scanstr)] = '\0';
                strncpy(tempname, scanstr, strlen(scanstr)+1);
              }
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> NAME : %s\n", tempname);
              }
            }
          }
        } else if (testkey[0] == 'r' || testkey[0] == 'R') { // REGISTER_ADDRESS
          strncpy(testkey, *(segments+i), 14);
          testkey[14] = '\0';
          if (strcmp(to_lowercase(testkey), "register_addr=") != 0) {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          } else {
            sscancount = sscanf(*(segments+i), "%*13c%*[=]%x", &tempregister_address);
            if (sscancount != 1) {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> REGISTER-ADDRESS : 0x%02X\n", tempregister_address);
              }
            }
          }
        } else if (testkey[0] == 't' || testkey[0] == 'T') { // TYPE
          strncpy(testkey, *(segments+i), 5);
          testkey[5] = '\0';
          if (strcmp(to_lowercase(testkey), "type=") != 0) {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
            }
            continue;
          } else {
            if (strcmp(to_lowercase(*(segments+i)), "type=unset_type") == 0) {
              temptype = UNSET_TYPE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> OPERATION TYPE: Not set (default).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "type=bus") == 0) {
              temptype = BUS;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> OPERATION TYPE: Accessing Bus (default Type).\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "type=device") == 0) {
              temptype = DEVICE;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> OPERATION TYPE: Accessing Specified Device directly.\n");
              }
            } else if (strcmp(to_lowercase(*(segments+i)), "type=register") == 0) {
              temptype = REGISTER;
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, " -> OPERATION TYPE: Accessing Specified Register of certain Device.\n");
              }
            } else {
              if (verbose) {
                fprintf(VERBOSE_OUT_STREAM, "Argument (\"%s\") is malformed, ignoring.\n", *(segments+i));
              }
              continue;
            }
          }
        }
      }
    } else {	// reset for next loop!
      is_twosegment_shortcut = false;
    }
    // catch some show-stoppers
    if (tempbus == -1) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring (No Bus specified).\n", lineno);
      }
      continue;
    }
    if (tempname == NULL) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring (No Name specified).\n", lineno);
      }
      continue;
    }
    if (temptype != UNSET_TYPE && temptype != BUS && temptype != DEVICE && temptype != REGISTER) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring (Invalid Access Type specified).\n", lineno);
      }
      continue;
    }
    if (tempmode != UNSET_MODE && tempmode != WORD && tempmode != BYTE && tempmode != RWBYTE && tempmode != WORD_PEC && tempmode != BYTE_PEC && tempmode != RWBYTE_PEC) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring (Invalid Access Mode specified).\n", lineno);
      }
      continue;
    }
    if (tempexpect != UNSET_EXPECT && tempexpect != VALUE && tempexpect != NONE && tempexpect != FAIL) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring (Invalid Access Expectation specified).\n", lineno);
      }
      continue;
    }
    // make sure we pass on valid char-array pointer to function
    if (tempexec_before == NULL) {
      tempexec_before = (char*)malloc(sizeof(char*)*2);
      sprintf(tempexec_before, "");
    }
    if (tempexec_after == NULL) {
      tempexec_after = (char*)malloc(sizeof(char*)*2);
      sprintf(tempexec_after, "");
    }

    insert_in_dynamic_shortcuts(tempbus, tempname, temptype, tempmode, tempexpect, tempdevice_address, tempregister_address, tempexpect_value, tempexec_before, tempexec_after);
  }

  fclose(fp);
  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "=====\n");
  }
  return;
}
#endif

void process_shortcut(shortcut_struct shortcut) {
#if IGNORE_COMPLEX_SHORTCUTS==0

  if (shortcut.type == BUS || shortcut.type == DEVICE || shortcut.type == REGISTER) {
    set_option(HAS_TYPE);
    _access_type = shortcut.type;
  }
  if (shortcut.mode == WORD || shortcut.mode == BYTE || shortcut.mode == RWBYTE) {
    set_option(HAS_MODE);
    _access_mode = shortcut.mode;
  }
  if (shortcut.expect == VALUE || shortcut.expect == NONE || shortcut.expect == FAIL) {
    set_option(HAS_EXPECT);
    _access_expect = shortcut.expect;
  }
  if (get_option(HAS_TYPE) == true && (shortcut.type == DEVICE || shortcut.type == REGISTER)) {
    if (shortcut.device_address > 0x02 && shortcut.device_address < 0x78) {
      set_option(HAS_DEVICE_ADDR);
      _device_address = shortcut.device_address;
    } else {
      if (!quiet) {
        fprintf(stderr, "Error: Invalid device address (\"0x%02X\").\n", shortcut.device_address);
      }
      clean_exit(1);
    }
  }
  if (get_option(HAS_TYPE) == true && get_option(HAS_DEVICE_ADDR) && shortcut.type == REGISTER) {
    set_option(HAS_REGISTER_ADDR);
    _register_address = shortcut.register_address;
  }
  if (get_option(HAS_EXPECT) == true && (shortcut.expect == VALUE && shortcut.expect_value != 0)) {
    _expect_value = shortcut.expect_value;
  } else if (get_option(HAS_EXPECT) == true && shortcut.expect_value != 0) {
    if (!quiet) {
      fprintf(stderr, "Error: Expected return value given, but the operation is expected to result in %s (instead of a value!)\n", (shortcut.expect==FAIL)?"Read-Error or errornous reading":"no output at all or none of relevance");
    }
    clean_exit(1);
  } else if (get_option(HAS_EXPECT) && shortcut.expect == VALUE) { // Value expected could be zero, but we can't check...
    _expect_value = shortcut.expect_value;
  }
  if (strlen(shortcut.exec_before) > 0) {
    set_option(HAS_EXEC_BEFORE);
    strcpy(_exec_before, shortcut.exec_before);
  }
  if (strlen(shortcut.exec_after) > 0) {
    set_option(HAS_EXEC_AFTER);
    strcpy(_exec_after, shortcut.exec_after);
  }
  // make sure, invalid combinations are ruled out
  if (get_option(HAS_TYPE) == true && _access_type == DEVICE && get_option(HAS_DEVICE_ADDR) == false) {
    if (!quiet) {
      fprintf(stderr, "Error: Invalid options have been given: (Access-Type: Device but no Device-Address specified!).\n");
    }
    clean_exit(1);
  }
  if (get_option(HAS_TYPE) == true && _access_type == REGISTER && (get_option(HAS_DEVICE_ADDR) == false || get_option(HAS_REGISTER_ADDR) == false)) {
    if (get_option(HAS_DEVICE_ADDR == false)) {
      if (!quiet) {
        fprintf(stderr, "Error: Invalid options have been given: (Access-Type: Register on Device but no Device-Address specified!).\n");
      }
    } else {
      if (!quiet) {
        fprintf(stderr, "Error: Invalid options have been given: (Access-Type: Register at Address but no Register-Address specified!).\n");
      }
    }
    clean_exit(1);
  }
#else
  return;
#endif
}


int get_shortcut(char *shortcutname) {

#if IGNORE_EXTERNAL_SHORTCUTS_FILE==0
  char *externalfile = getenv("IICSELECT_CONFIG");
  if (externalfile != NULL && strcmp(externalfile,"")!=0) {
    read_external_shortcuts(externalfile);

    for (int i = 0; i < get_dynamic_shortcuts_size(); i++) {
#if CASE_SENSITIVE==1
      if (strcmp((dynamic_shortcuts+i)->name, shortcutname) == 0) {
#else
      if (strcmp(to_lowercase((dynamic_shortcuts+i)->name), to_lowercase(shortcutname)) == 0) {
#endif
        process_shortcut(*(dynamic_shortcuts+i));
        return (dynamic_shortcuts+i)->bus;
      }
    }

#endif
  }

  int size = (sizeof(static_shortcuts)/(sizeof(shortcut_struct)));
  for (int i = 0; i < size; i++) {
#if CASE_SENSITIVE==1
    if (strcmp(static_shortcuts[i].name, shortcutname) == 0) {
#else
    if (strcmp(to_lowercase(static_shortcuts[i].name), to_lowercase(shortcutname)) == 0) {
#endif
      process_shortcut(static_shortcuts[i]);
      return static_shortcuts[i].bus;
    }
  }
  input_is_shortcut = false;
  return -1;
}

int validate_input(char *progname, char *argument) {

  // check for shortcuts or "-h" "--help" first

  if (strcmp(argument, "-h") == 0) {
    usage(progname);
  }
  if (strcmp(argument, "--help") == 0) {
    usage(progname);
  }


  long lnum;
  char *end;
  int is_shortcut = get_shortcut(argument);


  if (is_shortcut >= 0) {

    lnum = (long)is_shortcut;

  } else {

    lnum = strtol(argument, &end, 10);
    if (end == argument) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Error: Conversion of argument \"%s\" to device failed.\n", argument);
      }
      return -1;
    } else if (lnum == 0) {
      if (errno == EINVAL || errno == ERANGE) {
        if (verbose) {
          fprintf(VERBOSE_OUT_STREAM, "Error: Conversion of argument \"%s\" to device failed, or device number specified is too big!\n", argument);
        }
        return -1;
      }
    }
  }

  char iicdev[15];

  sprintf(iicdev, "i2c-%d", (int)lnum);
  DIR *d;
  struct dirent *dir;
  d = opendir("/dev");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (strcmp(dir->d_name, iicdev) == 0) {
        return (int)lnum;
      }
    }
    closedir(d);
  }
  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "Error: No device called \"%s\" found!\n", iicdev);
  }
  return -1;
}


void execute_command(char *cmd) {
  FILE *execFp;
  char output[MAX_SHELL_COMMAND_LENGHT];

  char *execCmd;
  strcpy(execCmd, cmd);

#if REDIRECT_EXTERNAL_COMMANDS_STDERR==1
  strcat(execCmd, " 2>&1");
#endif

  execFp = popen(execCmd, "r");
  if (execFp == NULL) {
    if (!quiet) {
      fprintf(stderr, "Failed to run command\n");
      return -1;
    }
  }

  while(fgets(output, sizeof(output), execFp) != NULL) {
    if (verbose) {
      fprintf(VERBOSE_OUT_STREAM, "  %s", output);
    }
  }

  pclose(execFp);

  return;
}


void clean_exit(int exitValue) {
  remove_pid(PIDFILE);
  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "PID-File (\"%s\") removed.\n", PIDFILE);
  }
  exit(exitValue);
}


int main(int argc, char *argv[]) {

  int pid = 0;

  if(check_pid(PIDFILE) == 0) {
    pid = write_pid(PIDFILE);
    if (pid == 0) {
      fprintf(stderr, "Error: Could not create PID-File.\n");
      clean_exit(1);
    } else {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "PID-File (PID: %d) at \"%s\" created.\n", pid, PIDFILE);
      }
    }
  } else {
    if (WAITTIME_ON_PIDLOCK > 0) {

      int waitcounter = WAITTIME_ON_PIDLOCK;

      while (waitcounter > 0 && pid == 0) {
        if (verbose) {
          fprintf(VERBOSE_OUT_STREAM, " !! Another Instance is already running (PID: %n).\n", read_pid(PIDFILE));
          fprintf(VERBOSE_OUT_STREAM, " !! Trying again in one Second (%n more attempt%s)\n", waitcounter, ((waitcounter>1)?"s":""));
        }
        sleep(1);
        waitcounter--;
        if (check_pid(PIDFILE) == 0) {
          pid = write_pid(PIDFILE);
          if (pid == 0) {
            fprintf(stderr, "Error: Could not create PID-File.\n");
            clean_exit(1);
          } else {
            if (verbose) {
              fprintf(VERBOSE_OUT_STREAM, "PID-File (PID: %d) at \"%s\" created.\n", pid, PIDFILE);
            }
            waitcounter = 0;
          }
        }
      }

    } else {
      fprintf(stderr, "Error: Another Instance is already running (PID: %n).\n", read_pid(PIDFILE));
      clean_exit(1);
    }

  }

  int file;
  char filename[20];
  int bus_no;
#if SKIP_IOCTL_FUNC_CHECK==0
  unsigned long funcs;
#endif

  if (argc != 2 && argc != 3) {
    usage(argv[0]);
  } else {
    if (argc == 2) {
      bus_no = validate_input(argv[0], argv[1]);
      if (bus_no < 0) {
        fprintf(stderr, "Error: Invalid argument supplied (\"%s\").\n", argv[1]);
        clean_exit(1);
      }
    } else {
      if (strcmp(argv[1],"-h")==0) {
        usage(argv[0]);
      } else if (strcmp(argv[1],"--help")==0) {
        usage(argv[0]);
      } else if ((strcmp(argv[1],"-v")==0) || (strcmp(argv[1],"--verbose")==0)) { // verbose
        verbose = true;
        fprintf(VERBOSE_OUT_STREAM, "Running in verbose mode...\n");
        bus_no = validate_input(argv[0], argv[2]);
        if (bus_no < 0) {
          fprintf(stderr, "Error: Invalid argument supplied (\"%s\").\n", argv[2]);
          clean_exit(1);
        }
      } else if ((strcmp(argv[1],"-q")==0) || (strcmp(argv[1],"--quiet")==0)) { // quiet
        quiet = true;
        bus_no = validate_input(argv[0], argv[2]);
        if (bus_no < 0) {
          clean_exit(1);
        }
      } else {
        usage(argv[0]);
      }
    }
  }

  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "Selecting I2C-Bus: %d\n", bus_no);
  }

  // Print summary over actions about to be taken
  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "=====\n");
    fprintf(VERBOSE_OUT_STREAM, "Supplied argument is detected as Shortcut: %s\n", (input_is_shortcut==true)?"Yes":"No");
    fprintf(VERBOSE_OUT_STREAM, "Shortcut was defined other than the default way: %s\n", (get_option(HAS_TYPE)==true)?"Yes":"No");
    if (get_option(HAS_TYPE)) {
      if (_access_type == BUS) {
        fprintf(VERBOSE_OUT_STREAM, " ...by its Bus-No that is (default).\n");
      } else if (_access_type == DEVICE) {
        fprintf(VERBOSE_OUT_STREAM, " ...by directly accessing a chosen Device at (0x%02X) on that very Bus that is.\n", _device_address);
      } else if (_access_type == REGISTER) {
        fprintf(VERBOSE_OUT_STREAM, " ...by directly accessing the Register (0x%02X) of a chosen Device at (0x%02X) on that very Bus that is.\n", _register_address, _device_address);
      }
    }
    fprintf(VERBOSE_OUT_STREAM, "Operation is to be performed other than in default mode: %s\n", (get_option(HAS_MODE)==true)?"Yes":"No");
    if (get_option(HAS_MODE)) {
      if (_access_mode == WORD) {
        fprintf(VERBOSE_OUT_STREAM, " ...by reading a DWord from the Bus that is.\n");
      } else if (_access_mode == BYTE) {
        fprintf(VERBOSE_OUT_STREAM, " ...by reading a single Byte from the Bus that is (default).\n");
      } else if (_access_mode == RWBYTE) {
        fprintf(VERBOSE_OUT_STREAM, " ...by writing to the Bus and reading a Byte back that is.\n");
      } else if (_access_mode == WORD_PEC) {
        fprintf(VERBOSE_OUT_STREAM, " ...by reading a DWord from the Bus with Packet Error Checking (PEC) that is.\n");
      } else if (_access_mode == BYTE_PEC) {
        fprintf(VERBOSE_OUT_STREAM, " ...by reading a single Byte from the Bus with Packet Error Checking (PEC) that is\n");
      } else if (_access_mode == RWBYTE_PEC) {
        fprintf(VERBOSE_OUT_STREAM, " ...by writing to the Bus and reading a Byte back with Packet Error Checking (PEC) that is.\n");
      }
    }
    fprintf(VERBOSE_OUT_STREAM, "Expect a certain return value to successfully complete the operation: %s\n", (get_option(HAS_EXPECT)==true)?"Yes":"No");
    if (get_option(HAS_EXPECT)) {
      if (_access_expect == VALUE) {
        fprintf(VERBOSE_OUT_STREAM, " ...required is a value of %i (0x%02X).\n", _expect_value, _expect_value);
      } else if (_access_expect == NONE) {
        fprintf(VERBOSE_OUT_STREAM, " ...required is any value, as long as there is one being returned (default).\n");
      } else if (_access_expect == FAIL) {
        fprintf(VERBOSE_OUT_STREAM, " ...expected is a Read-Error or errornous Result.\n");
      }
    }
    fprintf(VERBOSE_OUT_STREAM, "Is there a Shell-Command to be executed before performing the I2C-Bus operation: %s\n", (get_option(HAS_EXEC_BEFORE)==true)?"Yes":"No");
    if (get_option(HAS_EXEC_BEFORE)) {
      fprintf(VERBOSE_OUT_STREAM, " ( %s ) \n", _exec_before);
    }
    fprintf(VERBOSE_OUT_STREAM, "Is there a Shell-Command to be executed after successfully performing the I2C-Bus operation: %s\n", (get_option(HAS_EXEC_AFTER)==true)?"Yes":"No");
    if (get_option(HAS_EXEC_AFTER)) {
      fprintf(VERBOSE_OUT_STREAM, " ( %s ) \n", _exec_after);
    }
    fprintf(VERBOSE_OUT_STREAM, "=====\n");
  }

  snprintf(filename, 19, "/dev/i2c-%d", bus_no);

  if (input_is_shortcut && get_option(HAS_EXEC_BEFORE)) {
    if (verbose) {
      fprintf(VERBOSE_OUT_STREAM, "Executing command ( %s ): \n", _exec_before);
    }
    execute_command(_exec_before);
  }


  file = open(filename, O_RDONLY);
  if (verbose) {
    fprintf(VERBOSE_OUT_STREAM, "Opening device \"%s\"\n", filename);
  }
  if (file < 0) {
    if (!quiet) {
      fprintf(stderr, "Error: Open command on bus \"%d\" failed.\n", bus_no);
    }
    clean_exit(1);
  } else {
#if SKIP_IOCTL_FUNC_CHECK==0
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
      if (!quiet) {
        fprintf(stderr, "Error: Retrieving supported operations from device at \"%d\" failed.\n", bus_no);
      }
      clean_exit(1);
    }
#endif
    if (verbose) {
      fprintf(VERBOSE_OUT_STREAM, "Reading a single byte from the bus requested...\n");
    }
    int readbyte;
    readbyte = i2c_smbus_read_byte(file);
    if (verbose) {
      fprintf(VERBOSE_OUT_STREAM, "Data read: 0x%02X\n", readbyte);
    }
    close(file);


    if (input_is_shortcut && get_option(HAS_EXEC_AFTER)) {
      if (verbose) {
        fprintf(VERBOSE_OUT_STREAM, "Executing command ( %s ): \n", _exec_after);
      }
      execute_command(_exec_after);
    }

    free(dynamic_shortcuts);

    if (verbose) {
      fprintf(VERBOSE_OUT_STREAM, " --> Done!\n");
    }
    clean_exit(0);
  }
}
