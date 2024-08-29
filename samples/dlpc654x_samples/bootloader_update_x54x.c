#include "dlpc654x_sample.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#ifdef _WIN32
#include "3rdparty/dirent.h"
#else
#include <dirent.h>
#endif

#include "x54x_bootloader.h"

char* findFlashImage()
{
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(".")) == NULL)
    {
        printf("Failed to open current directory\n");
        return NULL;
    }

    chdir(".");

    char* flashImageFileName = NULL;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!flashImageFileName && strcmp(entry->d_name + strlen(entry->d_name) - 4, ".img") == 0)
        {
			flashImageFileName = malloc(strlen(entry->d_name) + 1);
			if (flashImageFileName)
			{
				strncpy(flashImageFileName, entry->d_name, strlen(entry->d_name));
                flashImageFileName[strlen(entry->d_name)] = '\0';
			}
        }
    }

    closedir(dir);
    return flashImageFileName;
}

int main(int argc, char** argv)
{
	printf("\n==================\n");
	printf("Updating bootloader\n");
	printf("===================\n\n");

	int retVal = doFastBootloaderUpdate(bootloaderData, sizeof(bootloaderData));

    if (!retVal)
    {
        printf("Flashing complete!\n");
    }
    else
    {
        printf("Flashing finished with error code %d\n", retVal);
    }

    printf("Press enter to close...");
    scanf("*");

    return retVal;
}