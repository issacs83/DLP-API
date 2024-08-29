#include "dlpc654x_sample.h"

void printUsage()
{
	printf("\n=======================================================\n");
	printf("Usage: dlpc654x_sample.exe /path/to/img/file [options]\n\n");
	printf("OPTIONS:\n");
	printf("\t-m\n");
	printf("\t\tUpdate modified sectors only\n");
	printf("\t-b\n");
	printf("\t\tProgram bootloader (skipped by default)\n");
	printf("=======================================================\n\n");
}

int main(int argc, char** argv)
{
	printUsage();
	if (argc < 2)
	{
		return 1;
	}

	uint8_t modifiedOnly = 0;
	uint8_t skipBootloader = 1;

	size_t i;

	for (i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'm':
				modifiedOnly = 1;
				break;
			case 'b':
				skipBootloader = 0;
				break;
			default:
				printUsage();
				return 1;
			}
		}
	}

	printf("\n=================================\n");
	printf("Updating flash\nUpdating modified sectors only? %d\nProgramming bootloader? %d\n", modifiedOnly, !skipBootloader);
	printf("=================================\n\n");

	return doFlashUpdate(argv[1], modifiedOnly, APPLICATION | skipBootloader);
}