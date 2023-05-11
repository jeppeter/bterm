#ifndef __BOGL_DEBUG_H_3DE65707D70ECCA3A4FBAAC6E9224053__
#define __BOGL_DEBUG_H_3DE65707D70ECCA3A4FBAAC6E9224053__



#define  BOGL_TRACE          4
#define  BOGL_DEBUG          3
#define  BOGL_INFO           2
#define  BOGL_WARN           1
#define  BOGL_ERROR          0

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

void bogl_output_buffer_debug(int level,char* file, int lineno,void* pbuf, int buflen,char* fmt, ...);
void bogl_output_debug(int level,char* file, int lineno,char* fmt, ...);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __BOGL_DEBUG_H_3DE65707D70ECCA3A4FBAAC6E9224053__ */
