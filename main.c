#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <windows.h>

//gcc -Wall -o0 -std=c99 -o gcodereader.exe main.c -Iinclude
//gcodereader.exe exemples_gcode/cube10.gcode


#define MAX_LINE_LENGTH 255
#define OUTPUT_RATE 4410 		// Output stream sampling rate

typedef struct {
	uint32_t n;	// Segment number
	float x;	// Segment's x end coordinates
	float y;	// Segment's y end coordinates
	float z;	// Segment's z end coordinates
	float speed;// Movement's speed
	int laserenabled; // Boolean
} Segment_t;

float currentX = 0;
float currentY = 0;
float currentZ = 0;

#define QUEUE_ELEMENTS 100
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)
Segment_t Queue[QUEUE_SIZE];
int QueueIn, QueueOut;


void QueueInit(void)
{
    QueueIn = QueueOut = 0;
}

int QueuePut(Segment_t new)
{
    if(QueueIn == (( QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
    {
        return -1; /* Queue Full*/
    }

    Queue[QueueIn] = new;

    QueueIn = (QueueIn + 1) % QUEUE_SIZE;

    return 0; // No errors
}

int QueueGet(Segment_t *old)
{
    if(QueueIn == QueueOut)
    {
        return -1; /* Queue Empty - nothing to get*/
    }

    *old = Queue[QueueOut];

    QueueOut = (QueueOut + 1) % QUEUE_SIZE;

    return 0; // No errors
}


void Gcommands(char* command) {
	//printf("Gcommand : %s", command);

	static float x = 0;
	static float y = 0;
	static float z = 0;
	static float speed = 0;
	static uint32_t n = 0;

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
			Segment_t newsegment;
			newsegment.n = n++;
			newsegment.x = x;
			newsegment.y = y;
			newsegment.z = z;
			newsegment.speed = speed;
			newsegment.laserenabled = laserenabled;
			while(QueuePut(newsegment) == -1) Sleep(10);
			//printf("Added segment %5d to queue.\n", newsegment.n);
			break;
	}

}


DWORD WINAPI MyThreadFunction( LPVOID osef ) {
	while (1) {
		Segment_t segment;
		while (QueueGet(&segment) == -1) Sleep(1);
		//printf("Read segment %5d from queue : ", segment.n);
		//printf("Move to  %8f, %8f, %8f at speed %f, laser %s\n", segment.x, segment.y, segment.z, segment.speed, segment.laserenabled ? "enabled" : "disabled");

		if (segment.z != currentZ)  {
			//printf("Z move\n");
			currentZ = segment.z;
		} else {

			float length = segment.speed / OUTPUT_RATE;
			float Dx = segment.x - currentX;
			float Dy = segment.y - currentY;
			float D = sqrt(pow(Dx, 2) + pow(Dy, 2));
			float A = length/D;
			uint32_t nsamples = D/length;

			while(nsamples--) {
				currentX += A * Dx;
				currentY += A * Dy;

				printf("%8f %8f\n", currentX, currentY);
			}
			currentX = segment.x;
			currentY = segment.y;
			printf("%8f %8f\n", currentX, currentY);
		}

	}
	return 0;
}

int main(int argc, char** argv) {

    // This thread will emulate I2S interrupts called at 44K/128 Hz
    DWORD dwThreadId;
    HANDLE hThread = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            MyThreadFunction,       // thread function name
            0,          			// argument to thread function 
            0,                      // use default creation flags 
            &dwThreadId);   // returns the thread identifier 

	//setlocale( LC_ALL, "" );

	QueueInit();

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
			//printf("Comment : %s", line+1);
		} else if (line[0] == 'G') {
			Gcommands(line);
		} else {
			//printf("Unexpected command : %s", line);
		}

	} while (fgetsreturn != NULL);

	fclose(gsource);

	return 0;
}