
#include "bogl-debug.h"
#include <stdlib.h>
#include <stdio.h>

static int st_bogl_debug_level = 0;
static FILE* st_bogl_debug_fp = NULL;


void bogl_output_debug(int level,char* file, int lineno,char* fmt, ...)
{
	return;
}