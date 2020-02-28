#include "example.h"

int main()
{
	while(1)
	{
		FILE *file = NULL;
		file = fopen("/tmp/pvz.txt", "a");
		if (file == NULL)
		{
			printf("Unable to open file");
			return -1;
		}
    
		fprintf(file, "Printing text every 3 seconds\n");
		fclose(file);
		sleep(3);
	}
	return 0;
}
