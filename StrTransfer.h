/************************************************************************/
/* 字符串格式转换                                                       */
/************************************************************************/

#pragma once

#include <wtypes.h>

#define SAFE_DELETE_ARRAY(p) if(p) { delete [] (p); (p) = 0; } 

//	多字节转换成宽字节
WCHAR* MByteToWChar(const char *pszStr);

//	宽字节转换成UTF8
char* WCharToUtf8(const WCHAR *pcwszStr);

//	多字节转换成UTF8
char* MByteToUtf8(const char *pszStr);
