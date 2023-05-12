
#include "bogl-debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

static int st_bogl_debug_level = BOGL_ERROR_LEVEL;
static FILE* st_bogl_debug_fp = NULL;

#define  BOGL_GETERRNO(ret) do{ ret = -errno ; if (ret == 0) {ret = -1;}} while(0)
#define  BOGL_SETERRNO(ret) do{ if (ret < 0) {errno = -ret;} else {errno = ret;}} while(0)


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

char* bogl_format_level(int level)
{
	if (level <= BOGL_ERROR_LEVEL) {
		return "ERROR";
	} else if (level == BOGL_WARN_LEVEL) {
		return "WARN";
	} else if (level == BOGL_INFO_LEVEL) {
		return "INFO";
	} else if (level == BOGL_DEBUG_LEVEL) {
		return "DEBUG";
	} else {
		return "TRACE";
	}
}

int bogl_format_time(time_t* ptm, char** ppstr,int *psize)
{
	struct tm restm, *prettm=NULL;
	char* pretstr = NULL;
	int retsize=0;
	int retlen = 0;
	size_t sret;
	int ret;
	if (ptm == NULL) {
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
		BOGL_SETERRNO(ret);
		return ret;
	}

	pretstr = *ppstr;
	retsize = *psize;

	memset(&restm,0,sizeof(restm));
	prettm = localtime_r(ptm,&restm);
	if (prettm == NULL) {
		BOGL_GETERRNO(ret);		
		goto fail;
	}

	if (pretstr == NULL || retsize == 0) {
		if (retsize == 0) {
			retsize = 4;
		}
		pretstr = malloc(retsize);
		if (pretstr == NULL) {
			BOGL_GETERRNO(ret);
			goto fail;
		}
	}


try_tm_again:
	memset(pretstr, 0, retsize);
	sret = strftime(pretstr, retsize - 1, "%Y-%m-%d %H:%M:%S", prettm);
	if (sret == 0) {
		retsize <<= 1;
		if (pretstr != NULL && pretstr != *ppstr) {
			free(pretstr);
		}
		pretstr = NULL;
		pretstr = malloc(retsize);
		if (pretstr == NULL) {
			BOGL_GETERRNO(ret);
			goto fail;
		}
		goto try_tm_again;
	}

	retlen = sret;

	if (*ppstr && *ppstr != pretstr) {
		free(*ppstr);
	}
	*ppstr = pretstr;
	*psize = retsize;
	return retlen;
fail:
	if (pretstr && pretstr != *ppstr) {
		free(pretstr);
	}
	pretstr = NULL;
	retsize = 0;
	BOGL_SETERRNO(ret);
	return ret;
}



int bogl_format_str(char** ppstr, int *psize,int level, char* file,int lineno,const char* fmt,va_list ap)
{
	char* curstr=NULL;
	int curlen=0;
	char* tmstr = NULL;
	int tmsize=0;
	time_t curtm;
	char* pretstr = NULL;
	int retsize=0;
	int retlen = 0;
	int ret;
	char* ptr=NULL;
	int leftlen;
    va_list oldap;
    va_copy(oldap, ap);
	
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

	pretstr = *ppstr;
	retsize = *psize;

	curtm = time(NULL);
	ret = bogl_format_time(&curtm,&tmstr,&tmsize);
	if (ret < 0) {
		BOGL_GETERRNO(ret);
		goto fail;
	}

	ret= asprintf(&curstr,"<%s>[%s]:",bogl_format_level(level),tmstr);
	if (ret < 0) {
		BOGL_GETERRNO(ret);
		goto fail;
	}
	curlen = ret;

	if (pretstr == NULL || retsize == 0) {
		if (retsize == 0) {
			retsize = 4;
		}
		pretstr = malloc(retsize);
		if (pretstr == NULL) {
			BOGL_GETERRNO(ret);
			goto fail;
		}
	}

try_format_again:
	memset(pretstr,0, retsize);
	if (retsize < curlen) {
		retsize <<= 1;
		if (pretstr && pretstr != *ppstr) {
			free(pretstr);
		}
		pretstr = NULL;
		pretstr = malloc(retsize);
		if (pretstr == NULL) {
			BOGL_GETERRNO(ret);
			goto fail;
		}
		goto try_format_again;
	}
	memcpy(pretstr,curstr,curlen);

	ptr = &(pretstr[curlen]);
	leftlen = retsize - curlen;
	if (leftlen < 1) {
		retsize <<= 1;
		if (pretstr && pretstr != *ppstr) {
			free(pretstr);
		}
		pretstr = NULL;
		pretstr = malloc(retsize);
		if (pretstr == NULL) {
			BOGL_GETERRNO(ret);
			goto fail;
		}
		goto try_format_again;		
	}

	va_copy(ap, oldap);
	ret = snprintf(ptr,leftlen-1,fmt,ap);
	if (ret >= (leftlen - 1)) {
		retsize <<= 1;
		if (pretstr && pretstr != *ppstr) {
			free(pretstr);
		}
		pretstr = NULL;
		pretstr = malloc(retsize);
		if (pretstr == NULL) {
			BOGL_GETERRNO(ret);
			goto fail;
		}
		goto try_format_again;
	} else if (ret < 0) {
		BOGL_GETERRNO(ret);
		goto fail;
	}
	retlen = curlen + ret;

	if (*ppstr && *ppstr != pretstr) {
		free(*ppstr);
	}
	*ppstr = pretstr;
	*psize = retsize;

	bogl_format_time(NULL,&tmstr,&tmsize);

	if (curstr) {
		free(curstr);
	}
	curstr = NULL;


	return retlen;
fail:
	if (pretstr && pretstr != *ppstr) {
		free(pretstr);
	}
	pretstr = NULL;
	retsize = 0;

	bogl_format_time(NULL,&tmstr,&tmsize);

	if (curstr) {
		free(curstr);
	}
	curstr = NULL;
	BOGL_SETERRNO(ret);
	return ret;
}



void bogl_output_buffer_debug(int level,char* file, int lineno,void* pbuf, int buflen,char* fmt, ...)
{
	char* fmtstr =NULL;
	int fmtsize=0;
	int wlen=0;
	int totallen = 0;
	va_list ap;
	unsigned char* ptr=NULL;
	int i;
	int lasti;
	int ret;
	bolg_output_init();
	if (st_bogl_debug_fp != NULL && level <= st_bogl_debug_level) {
		va_start(ap,fmt);
		ret = bogl_format_str(&fmtstr,&fmtsize,level,file,lineno,fmt,ap);
		if (ret >= 0)  {
			wlen = ret;
			ret = fwrite(fmtstr,wlen,1,st_bogl_debug_fp);
			if (ret == 1) {
				totallen += wlen;
				ret = fprintf(st_bogl_debug_fp," buffer [%p] size[0x%x:%d]", pbuf,buflen,buflen);
				if (ret > 0 ){
					totallen += ret;
					ptr = (unsigned char*) pbuf;
					lasti = 0;
					for(i=0;i<buflen;i++) {
						/*now we should*/
						if ((i%16) == 0) {
							if (i > 0) {
								ret = fprintf(st_bogl_debug_fp,"    ");
								if (ret > 0) {
									totallen += ret;
								}
								while(lasti != i) {
									if (ptr[lasti] >= 0x20 && ptr[lasti] <= 0x7e) {
										ret = fprintf(st_bogl_debug_fp,"%c", ptr[lasti]);
									} else {
										ret = fprintf(st_bogl_debug_fp, ".");
									}
									if (ret > 0) {
										totallen += ret;
									}
									lasti ++;
								}
							}
							ret = fprintf(st_bogl_debug_fp,"\n");
							if (ret > 0) {
								totallen += ret;
							}
						}
						ret = fprintf(st_bogl_debug_fp, " 0x%02x", ptr[i]);
						if (ret > 0) {
							totallen += ret;
						}
					}

					if (i != lasti) {
						while((i % 16) != 0) {
							ret = fprintf(st_bogl_debug_fp,"     ");
							if (ret > 0) {
								totallen += ret;
							}
							i ++ ;
						}
						ret = fprintf(st_bogl_debug_fp, "    ");
						if (ret > 0) {
							totallen += ret;
						}

						while(lasti < buflen) {
							if (ptr[lasti] >= 0x20 && ptr[lasti] <= 0x7e) {
								ret = fprintf(st_bogl_debug_fp, "%c", ptr[lasti]);
							} else {
								ret = fprintf(st_bogl_debug_fp, ".");
							}
							if (ret > 0) {
								totallen += ret;
							}
							lasti ++;
						}
						ret = fprintf(st_bogl_debug_fp, "\n");
						if (ret > 0) {
							totallen += ret;
						}
					}
				}
			}
		}

		if (fmtstr) {
			free(fmtstr);
		}
		fmtstr = NULL;
		fmtsize = 0;
		fflush(st_bogl_debug_fp);
	}
	return;
}

void bogl_output_debug(int level,char* file, int lineno,char* fmt, ...)
{
	char* fmtstr =NULL;
	int fmtsize=0;
	va_list ap;
	int totallen = 0;
	int ret;
	int wlen;
	bolg_output_init();
	if (st_bogl_debug_fp != NULL && level <= st_bogl_debug_level) {
		va_start(ap,fmt);
		ret = bogl_format_str(&fmtstr,&fmtsize,level,file,lineno,fmt,ap);
		if (ret > 0) {
			wlen = ret;
			ret = fwrite(fmtstr,wlen,1, st_bogl_debug_fp);
			if (ret == 1) {
				totallen += wlen;
				ret = fprintf(st_bogl_debug_fp,"\n");
				if (ret > 0) {
					totallen += ret;
				}
			}
		}
		if (fmtstr) {
			free(fmtstr);
		}
		fmtstr = NULL;
		fmtsize = 0;
		fflush(st_bogl_debug_fp);
	}
	return;
}