
#include "StrTransfer.h"

// 多字节转换成宽字节
WCHAR* MByteToWChar(const char *pszStr)
{
	DWORD dwSize = ::MultiByteToWideChar(CP_ACP, 0, pszStr, -1, NULL, 0);
	WCHAR *pwszStr = new WCHAR[dwSize + 1];
	::MultiByteToWideChar (CP_ACP, 0, pszStr, -1, pwszStr, dwSize);
	pwszStr[dwSize] = 0;
	return pwszStr;
}

// 宽字节转换成UTF8
char* WCharToUtf8(const WCHAR *pcwszStr)
{
	DWORD dwSize = ::WideCharToMultiByte(CP_UTF8, 0, pcwszStr, -1, NULL, 0, NULL, NULL);
	char *pszUtf8 = new char[dwSize + 1];
	::WideCharToMultiByte(CP_UTF8, NULL, pcwszStr, -1, pszUtf8, dwSize, NULL, FALSE);
	pszUtf8[dwSize] = 0;
	return pszUtf8;
}

// 多字节转换成UTF8
char* MByteToUtf8(const char *pszStr)
{
	WCHAR *wStrTmp = MByteToWChar(pszStr);
	char *pUtf8 = WCharToUtf8(wStrTmp);
	SAFE_DELETE_ARRAY(wStrTmp);
	return pUtf8;
}
