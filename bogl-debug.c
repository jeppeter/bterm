
#include "bogl-debug.h"
#include <stdlib.h>
#include <stdio.h>

static int st_bogl_debug_level = BOGL_ERROR;
static FILE* st_bogl_debug_fp = NULL;


int bolg_output_init(void)
{
	static int st_bogl_debug_inited = 0;
	char* envstr=NULL;
	if (st_bogl_debug_inited == 0) {
		st_bogl_debug_inited = 1;
		envstr = getenv("BTERM_DEBUG_LEVEL");
		if (envstr != NULL) {
			st_bogl_debug_level = atoi(envstr);
		}

		envstr = getenv("BTERM_DEBUG_FILE");
		if (envstr != NULL) {
			st_bogl_debug_fp = fopen(envstr,"a+");
		}
	}
	return 0;
}



int format_bogl_level(char** ppstr, int *psize,int level, char* file,int lineno,const char* fmt,va_list ap)
{
	char* curstr=NULL;
	char* pretstr = NULL;
	int retsize=0;
	int retlen = 0;
	int ret;
	if (fmt == NULL) {
		if (ppstr && *ppstr) {
			free(*ppstr);
			*ppstr = NULL;
		}

		if (psize) {
			*psize = 0;
		}

		return 0;
	}

	if (ppstr == NULL || psize == NULL) {
		ret = -EINVAL;
		goto fail;
	}




	return retlen;
fail:
	if (curstr) {
		free(curstr);
	}
	curstr = NULL;
	errno = ret;
	return ret;
}

void bogl_output_buffer_debug(int level,char* file, int lineno,void* pbuf, int buflen,char* fmt, ...)
{
	bolg_output_init();
	if (st_bogl_debug_fp != NULL && level <= st_bogl_debug_level) {

	}
	return
}

void bogl_output_debug(int level,char* file, int lineno,char* fmt, ...)
{
	bolg_output_init();
	if (st_bogl_debug_fp != NULL && level <= st_bogl_debug_level) {
		
	}
	return;
}