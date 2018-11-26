#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>


#define CASE_SENSITIVE 1	  		// 1 = True, 0 = False (Case insensitive)

#define USE_TEXT_FORMATING 0	  		// 1 = True, 0 = False (Only standard output)

#define SKIP_IOCTL_FUNC_CHECK 1	 		// 1 = True, 0 = False (Do some basic checks)

#define IGNORE_EXTERNAL_SHORTCUTS_FILE 0	// 1 = True, 0 == False (Do read external files if specified)

#define VERBOSE_OUT_STREAM stderr		// "stderr" or "stdout"

#define MAX_SHORTCUT_LENGTH 100			// Maximum length of chars for shortcut names



typedef struct { int bus; char name[MAX_SHORTCUT_LENGTH]; } shortcut_struct;


// convenience shortcuts, list aliases here:
// shortcuts can also be placed in file
// (conflicting shortcuts from file overwrite
//  shortcuts specified here).
// the file needs to be referenced as environment-
// variable called IICSELECT_CONFIG at runtime
// reading shortcuts at runtime from a file can
// be globally disabled by setting the macro
// IGNORE_EXTERNAL_SHORTCUTS_FILE to 1

shortcut_struct static_shortcuts[] = {

	{ .bus = 3, .name = "display" },
	{ .bus = 4, .name = "camera" },
	{ .bus = 5, .name = "ir-sensor" }

};


shortcut_struct *dynamic_shortcuts = NULL;
int dynamic_shortcuts_size = 0;


#define bool int
#define true 1
#define false 0


bool verbose = false;
bool quiet = false;


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
	exit(127);
}

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
		strcpy(&(_dynamic_shortcuts+i)->name, (dynamic_shortcuts+i)->name);
	}
	dynamic_shortcuts = _dynamic_shortcuts;
	dynamic_shortcuts_size++;
}

void insert_in_dynamic_shortcuts(int _bus, char *_name) {
	if (dynamic_shortcuts_size < 1) {
		init_dynamic_shortcuts();
	} else {
		grow_dynamic_shortcuts();
	}
	(dynamic_shortcuts+(dynamic_shortcuts_size-1))->bus = _bus;
	strcpy(&(dynamic_shortcuts+(dynamic_shortcuts_size-1))->name, _name);
}


#if CASE_SENSITIVE==1
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
#endif

#if IGNORE_EXTERNAL_SHORTCUTS_FILE==0
void read_external_shortcuts(char *filename) {
	FILE *fp;
	char *line = NULL;
	size_t lineno = 0;
	size_t len = 0;
	ssize_t read;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Error: File \"%s\" could not be opened!\n", filename);
		return;
	}

	if (verbose) {
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
		if (commas != 1) {
			if (verbose) {
				fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring.\n", lineno);
			}
			continue;
		}
		int commaat = -1;
		p = NULL;
		p = strchr(parsestr, ',');
		if (p != NULL) {
			commaat = p - parsestr;
		}
		char busstr[commaat+2];
		char namestr[end-commaat+1];
		snprintf(busstr, commaat+1, "%s", parsestr);
		snprintf(namestr, (end-commaat-1), "%s", parsestr+commaat+1);
		busstr[commaat+1] = '\0';
		namestr[end-commaat] = '\0';

		int tempbus;
		char tempname[MAX_SHORTCUT_LENGTH];

		int scanresult;

		//if there is a '=' inside the string, skip the first part
		if (strchr(busstr, '=') != NULL) {
			char _busstr[commaat+2];
			int position=-1;
			int i = 0;
			for (i=0; i<(commaat+1); i++) {
				if (position >= 0) {
					_busstr[i-position] = busstr[i];
				} else {
					if (busstr[i] == '=') { position = i+1; }
				}
			}
			_busstr[i-position] = '\0';
			strcpy(busstr, _busstr);
		}
		if (strchr(namestr, '=') != NULL) {
			char _namestr[end-commaat+1];
			int position=-1;
			int i = 0;
			for (i=0; i<(end-commaat); i++) {
				if (position >= 0) {
					_namestr[i-position] = namestr[i];
				} else {
					if (namestr[i] == '=') { position = i+1; }
				}
			}
			_namestr[i-position] = '\0';
			strcpy(namestr, _namestr);
		}
		// check that there are only digits left in the busstr
		int errors = 0;
		for (int i = 0; i < strlen(busstr); i++) {
			if (busstr[i] < 48 || busstr[i] > 57) {
				errors++;
			}
		}
		if (errors != 0) {
			if (verbose) {
				fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring.\n", lineno);
			}
			continue;
		}
		tempbus = (int)strtol(busstr, NULL, 10);
		scanresult = sscanf(namestr, "%*[\'\"]%[^\'\"]s", tempname);
		if (scanresult != 1) {
			if (verbose) {
				fprintf(VERBOSE_OUT_STREAM, "Line %d is malformed, ignoring.\n", lineno);
			}
			continue;
		} else {
			// now insert tempbus and tempname into shortcuts struct
			if (verbose) {
				fprintf(VERBOSE_OUT_STREAM, "Found entry for bus %d with name \"%s\"\n", tempbus, tempname);
			}
			// insert entry into array of dynamic shortcuts
			insert_in_dynamic_shortcuts(tempbus, tempname);
		}	
	}

	fclose(fp);
	return;
}
#endif

int get_shortcut(char *shortcutname) {

#if IGNORE_EXTERNAL_SHORTCUTS_FILE==0
	char *externalfile = getenv("IICSELECT_CONFIG");
	if (externalfile != NULL && strcmp(externalfile,"")!=0) {
		read_external_shortcuts(externalfile);
	}

	for (int i = 0; i < get_dynamic_shortcuts_size(); ++i) {
#if CASE_SENSITIVE==1
		if (strcmp((dynamic_shortcuts+i)->name, shortcutname) == 0) {
#else
		if (strcmp(to_lowercase((dynamic_shortcuts+i)->name), to_lowercase(shortcutname)) == 0) {
#endif
			return (dynamic_shortcuts+i)->bus;
		}
	}

#endif

	int size = (sizeof(static_shortcuts)/8);
	for (int i = 0; i < size; i++) {
#if CASE_SENSITIVE==1
		if (strcmp(static_shortcuts[i].name, shortcutname) == 0) {
#else
		if (strcmp(to_lowercase(static_shortcuts[i].name), to_lowercase(shortcutname)) == 0) {
#endif
			return static_shortcuts[i].bus;
		}
	}
	return -1;
}

int validate_input(char *progname, char *argument) {

	// check for shortcuts or "-h" "--help" first

	if (strcmp(argument, "-h") == 0) { usage(progname); }
	if (strcmp(argument, "--help") == 0) { usage(progname); }


	long lnum;
	char *end;
	int is_shortcut = get_shortcut(argument);


	if (is_shortcut >= 0) {

		lnum = (long)is_shortcut;

	} else {

		lnum = strtol(argument, &end, 10);
		if (end == argument) {
			if (verbose) { fprintf(VERBOSE_OUT_STREAM, "Error: Conversion of argument \"%s\" to device failed.\n", argument); }
			return -1;
		} else if (lnum == 0) {
			if (errno == EINVAL || errno == ERANGE) {
				if (verbose) { fprintf(VERBOSE_OUT_STREAM, "Error: Conversion of argument \"%s\" to device failed, or device number specified is too big!\n", argument); }
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
	if (verbose) { fprintf(VERBOSE_OUT_STREAM, "Error: No device called \"%s\" found!\n", iicdev); }
	return -1;
}


int main(int argc, char *argv[]) {

	int file;
	char filename[20];
	int bus_no;
	unsigned long funcs;

	if (argc != 2 && argc != 3) {
		usage(argv[0]);
	} else {
		if (argc == 2) {
			bus_no = validate_input(argv[0], argv[1]);
			if (bus_no < 0) {
				fprintf(stderr, "Error: Invalid argument supplied (\"%s\").\n", argv[1]);
				exit(1);
			}
		} else {
			if (strcmp(argv[1],"-h")==0) { usage(argv[0]);
			} else if (strcmp(argv[1],"--help")==0) { usage(argv[0]);
			} else if ((strcmp(argv[1],"-v")==0) || (strcmp(argv[1],"--verbose")==0)) { // verbose
				verbose = true;
				fprintf(VERBOSE_OUT_STREAM, "Running in verbose mode...\n");
				bus_no = validate_input(argv[0], argv[2]);
				if (bus_no < 0) {
					fprintf(stderr, "Error: Invalid argument supplied (\"%s\").\n", argv[2]);
					exit(1);
				}
			} else if ((strcmp(argv[1],"-q")==0) || (strcmp(argv[1],"--quiet")==0)) { // quiet
				quiet = true;
				bus_no = validate_input(argv[0], argv[2]);
				if (bus_no < 0) {
					exit(1);
				}
			} else { usage(argv[0]); }
		}
	}

	
	if (verbose) {
		printf("Selecting I2C-Bus: %d\n", bus_no);
	}	

	snprintf(filename, 19, "/dev/i2c-%d", bus_no);

	file = open(filename, O_RDONLY);
	if (verbose) { fprintf(VERBOSE_OUT_STREAM, "Opening device \"%s\"\n", filename); }
	if (file < 0) {
		if (!quiet) {
			fprintf(stderr, "Error: Open command on bus \"%d\" failed.\n", bus_no);
		}
		exit(1);
	} else {
#if SKIP_IOCTL_FUNC_CHECK==0
		if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
			if (!quiet) {
				fprintf(stderr, "Error: Retrieving supported operations from device at \"%d\" failed.\n", bus_no);
			}
			exit(1);
		}
#endif
		if (verbose) {
			fprintf(VERBOSE_OUT_STREAM, "Reading a single byte from the bus requested...\n");
		}
		int readbyte;
		readbyte = i2c_smbus_read_byte(file);
		if (verbose) { fprintf(VERBOSE_OUT_STREAM, "Data read: 0x%02X\n", readbyte); }
		close(file);
		if (verbose) { fprintf(VERBOSE_OUT_STREAM, " --> Done!\n"); }
		exit(0);
	}	
}
