#include <stdio.h>
#include"capture.h"
#include"flow.h"

int main(int argc,char **argv)
{
	if(argc < 2)
	{
	printf("usage:%s<interface>\n",argv[0]);
	return -1;
	}
    printf("Jitter analyzer started\n");
	
	if (start_capture(argv[1], argc >= 3 ? argv[2] : NULL) != 0)
	{
		fprintf(stderr, "failed to start packet capture\n");
		return -1;
	}
	print_all_flow_stats();
    return 0;
}
