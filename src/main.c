#include <stdio.h>
#include"capture.h"

int main(int argc,char **argv)
{
	if(argc < 2)
	{
	printf("usage:%s<interface>\n",argv[0]);
	return -1;
	}
    printf("Jitter analyzer started\n");
	
	if(start_capture(argv[1])!=0)
	{
		fprintf(stderr, "failed to start packet capture\n");
		return -1;
	}
    return 0;
}
