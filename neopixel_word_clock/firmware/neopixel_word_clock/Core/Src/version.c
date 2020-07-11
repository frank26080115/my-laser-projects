/*
 * version.c
 *
 *  Created on: Jul 7, 2020
 *      Author: frank
 *
 *  Prints the compiler information
 */

#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

char* version_string(char* buff)
{
	int idx = 0;
	idx += sprintf(&buff[idx], "NeoPixel Word Clock, ");
	idx += sprintf(&buff[idx], "Compiled on " __DATE__ ", " __TIME__", ");
	#ifdef __GNUC__
	idx += sprintf(&buff[idx], "GNU C %d", __GNUC__);
	#ifdef __GNUC_MINOR__
	idx += sprintf(&buff[idx], ".%d", __GNUC_MINOR__);
	#ifdef __GNUC_PATCHLEVEL__
	idx += sprintf(&buff[idx], ".%d", __GNUC_PATCHLEVEL__);
	#endif
	#endif
	#else
	idx += sprintf(&buff[idx], "unknown compiler\r\n");
	#endif
	return (char*)buff;
}
