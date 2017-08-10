#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 255

void Gcommands(char* command) {
	//printf("Gcommand : %s", command);

	static float x = 0;
	static float y = 0;
	static float z = 0;
	static float speed = 0;

	int numcommand = atoi(command+1);
	//printf("atoi command : %d\n", numcommand);
	int laserenabled = 1;
	switch(numcommand) {
		case 0 :
			laserenabled = 0;
		case 1 :
			while (*command !='\n' && *command && *command !=';') {
				switch(*command) {
					case 'X' : 
						x = (float)strtod(command+1, NULL);
						break;

					case 'Y':
						y = (float)strtod(command+1, NULL);
						break;

					case 'Z':
						z = (float)strtod(command+1, NULL);
						break;

					case 'F':
						speed = (float)strtod(command+1, NULL);
						break;

					default:
						break;
				}
				command++;
			}
			printf("Move to  %8f, %8f, %8f at speed %f, laser %s\n", x, y, z, speed, laserenabled ? "enabled" : "disabled");
			break;
	}

}

int main(int argc, char** argv) {

	//setlocale( LC_ALL, "" );

	if (argc != 2) {
		printf("Usage : %s fichier.gcode\n\n", argv[0], argv[1]);
		return 1;
	}

	FILE* gsource = fopen(argv[1], "r");

	if (gsource == NULL) {
		puts("Impossible d'ouvrir le fichier");
		return 1;
	}

	char currentLine[MAX_LINE_LENGTH];
	char* fgetsreturn;
	do {
		fgetsreturn = fgets(currentLine, MAX_LINE_LENGTH, gsource);
		//printf("Current line : %s", currentLine);
		char* line = currentLine;
		while(*line == ' ') line++;

		if (line[0] == ';') {
			printf("Comment : %s", line+1);
		} else if (line[0] == 'G') {
			Gcommands(line);
		} else {
			printf("Unexpected command : %s", line);
		}

	} while (fgetsreturn != NULL);

	fclose(gsource);

	return 0;
}