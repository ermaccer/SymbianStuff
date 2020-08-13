#ifndef __CW_ARMCC_WIN32_OFF_H__
#define __CW_ARMCC_WIN32_OFF_H__


#undef _WIN32
#undef WIN32
#undef __i386__

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#define C_ONLY

#endif
