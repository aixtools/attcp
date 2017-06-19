
/* Program to read on stdin forever */

#include <stdio.h>

int main()
{
	int ret = 0;
	int buffer_size = 1024*1024;
	char *buffer = (char*)malloc(buffer_size);
	if (NULL == buffer) {
		exit(1);
	}
	while (1) {
/*
		ret = fread(buffer,1,buffer_size,stdin);

 * check to syscall read() from library call fread()
 */
		ret = read(0, buffer, buffer_size);
		if (0 == ret) {
			break;
		}
	}
	return 0;
}
