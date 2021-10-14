//
// -*- coding: utf-8 -*-
// $Date: 2021/10/13 22:43:05 $
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char buff[256];
	getcwd(buff, 256);
	printf("cd %s\n", buff);
	strcat(buff, "\\");

	for(int i = 0; i < argc; i++){
		printf("%2d %s\n", i, argv[i]);
		/*
		if(strncmp(buff, argv[i], strlen(buff)) == 0){
			printf("  => %s\n", argv[i]+strlen(buff));
		}else{
			;;;
		}
		*/
	}
}
