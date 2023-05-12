#ifndef __BOGL_DEBUG_H_3DE65707D70ECCA3A4FBAAC6E9224053__
#define __BOGL_DEBUG_H_3DE65707D70ECCA3A4FBAAC6E9224053__



#define  BOGL_TRACE_LEVEL          4
#define  BOGL_DEBUG_LEVEL          3
#define  BOGL_INFO_LEVEL           2
#define  BOGL_WARN_LEVEL           1
#define  BOGL_ERROR_LEVEL          0

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

void bogl_output_buffer_debug(int level,char* file, int lineno,void* pbuf, int buflen,char* fmt, ...);
void bogl_output_debug(int level,char* file, int lineno,char* fmt, ...);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#define BOGL_DEBUG(...)                         do{bogl_output_debug(BOGL_DEBUG_LEVEL,__FILE__,__LINE__,__VA_ARGS__);}while(0)
#define BOGL_BUFFER_DEBUG(ptr,sz,...)           do{bogl_output_buffer_debug(BOGL_DEBUG_LEVEL,__FILE__,__LINE__,(void*)(ptr),(int)(sz),__VA_ARGS__);}while(0)

#endif /* __BOGL_DEBUG_H_3DE65707D70ECCA3A4FBAAC6E9224053__ */
