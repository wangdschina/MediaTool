#pragma once

#include <string>
#include <vector>
#include <algorithm>

#if defined(UNICODE) || defined(_UNICODE)
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

class CStringUtil
{
public:
	CStringUtil() {}
	~CStringUtil() {}

	static tstring Format(LPCTSTR lpszFmt, ...)
	{
		if (NULL == lpszFmt)
			return tstring();

		va_list vMarker; 
		va_start(vMarker, lpszFmt);
		int nLen = _vsntprintf(NULL, 0, lpszFmt, vMarker) + 1;
		tstring strRet;
		std::vector<TCHAR> vBuffer(nLen, '\0');
		_vsntprintf(&vBuffer[0], vBuffer.size(), lpszFmt, vMarker);
		strRet.assign(&vBuffer[0]);
		va_end(vMarker);

		return strRet;
	}

	static tstring GetBetweenString(const TCHAR* lpszStr, const TCHAR* lpszStart, const TCHAR* lpszEnd)
	{
		tstring strRet;

		if (NULL == lpszStr || NULL == lpszStart || NULL == lpszEnd)
			return tstring();

		size_t startLen = _tcslen(lpszStart);

		const TCHAR* p1 = _tcsstr(lpszStr, lpszStart);
		if (NULL == p1)
			return tstring();

		const TCHAR* p2 = _tcsstr(p1 + startLen, lpszEnd);
		if (NULL == p2)
			return tstring();

		size_t len = p2 - (p1 + startLen);
		if (len <= 0)
			return tstring();

		TCHAR* lpszRet = new TCHAR[len + 1];
		if (NULL == lpszRet)
			return tstring();

		memset(lpszRet, 0, sizeof(TCHAR) * (len + 1));
		_tcsncpy(lpszRet, p1 + startLen, len);
		strRet = lpszRet;
		delete[] lpszRet;

		return strRet;
	}

	static tstring ReplaceAll(const tstring& str, const tstring& strOld, const tstring& strNew, int* pNReplaced = NULL)
	{
		if (pNReplaced != NULL)
			*pNReplaced = 0;

		tstring strRet = str;
		while(true) 
		{
			tstring::size_type pos(0);
			if ((pos = strRet.find(strOld)) != tstring::npos)
			{
				strRet.replace(pos, strOld.length(), strNew);
				if (pNReplaced != NULL)
					*pNReplaced++;
			}
			else
			{
				break;
			}
		}
		return strRet;
	}

	static tstring ReplaceAllDistinct(const tstring& str, const tstring& strOld, const tstring& strNew, int* pNReplaced = NULL)
	{
		if (pNReplaced != NULL)
			*pNReplaced = 0;

		tstring strRet = str;
		for(tstring::size_type pos(0); pos!= tstring::npos; pos += strNew.length())
		{
			if ((pos = strRet.find(strOld, pos)) != tstring::npos)
			{
				strRet.replace(pos, strOld.length(), strNew);
				if (pNReplaced != NULL)
					*pNReplaced++;
			}
			else
			{
				break;
			}
		}
		return strRet;
	}

	static tstring ToLower(const tstring& str)
	{
		tstring strRet = str;
		std::transform(strRet.begin(), strRet.end(), strRet.begin(), tolower);
		return strRet;
	}

	static tstring ToUpper(const tstring& str)
	{
		tstring strRet = str;
		std::transform(strRet.begin(), strRet.end(), strRet.begin(), toupper);
		return strRet;
	}

	static int ToInt(const tstring& str)
	{
		if (str.empty())
			return 0;

		return _ttoi(str.c_str());
	}

	static bool ToBool(const tstring& str)
	{
		if (str.empty())
			return false;

		return ToInt(str) != 0;
	}

	static double ToFloat(const tstring& str)
	{
		if (str.empty()) 
			return 0;

		return _ttof(str.c_str());
	}

	static std::string TStrToAnsi(const tstring& strT)
	{
#if defined(UNICODE) || defined(_UNICODE)
		return UnicodeToAnsi(strT);
#else
		return strT;
#endif
	}

	static std::string TStrToUtf8(const tstring& strT)
	{
#if defined(UNICODE) || defined(_UNICODE)
		return UnicodeToUtf8(strT);
#else
		return AnsiToUtf8(strT);
#endif
	}

	static std::wstring TStrToUnicode(const tstring& strT)
	{
#if defined(UNICODE) || defined(_UNICODE)
		return strT;
#else
		return AnsiToUnicode(strT);
#endif
	}

	static tstring AnsiToTStr(const std::string& strAnsi)
	{
#if defined(UNICODE) || defined(_UNICODE)
		return AnsiToUnicode(strAnsi);
#else
		return strAnsi;
#endif
	}

	static tstring Utf8ToTStr(const std::string& strUtf8)
	{
#if defined(UNICODE) || defined(_UNICODE)
		return Utf8ToUnicode(strUtf8);
#else
		return Utf8ToAnsi(strUtf8);
#endif
	}

	static tstring UnicodeToTStr(const std::wstring& strUnicode)
	{
#if defined(UNICODE) || defined(_UNICODE)
		return strUnicode;
#else
		return UnicodeToAnsi(strUnicode);
#endif
	}

	static std::string FormatA(LPCSTR lpszFmt, ...)
	{
		if (NULL == lpszFmt)
			return std::string();

		va_list vMarker;
		va_start(vMarker, lpszFmt);
		int nLen = _vsnprintf(NULL, 0, lpszFmt, vMarker) + 1;
		std::string strRet;
		std::vector<CHAR> vBuffer(nLen, '\0');
		_vsnprintf(&vBuffer[0], vBuffer.size(), lpszFmt, vMarker);
		strRet.assign(&vBuffer[0]);
		va_end(vMarker);

		return strRet;
	}

	static std::wstring FormatW(LPCWSTR lpszFmt, ...)
	{
		if (NULL == lpszFmt)
			return std::wstring();

		va_list vMarker;
		va_start(vMarker, lpszFmt);
		int nLen = _vsnwprintf(NULL, 0, lpszFmt, vMarker) + 1;
		std::wstring strRet;
		std::vector<WCHAR> vBuffer(nLen, '\0');
		_vsnwprintf(&vBuffer[0], vBuffer.size(), lpszFmt, vMarker);
		strRet.assign(&vBuffer[0]);
		va_end(vMarker);

		return strRet;
	}

	static std::string GetBetweenStringA(const CHAR* lpszStr, const CHAR* lpszStart, const CHAR* lpszEnd)
	{
		std::string strRet;

		if (NULL == lpszStr || NULL == lpszStart || NULL == lpszEnd)
			return std::string();

		size_t startLen = strlen(lpszStart);

		const CHAR* p1 = strstr(lpszStr, lpszStart);
		if (NULL == p1)
			return std::string();

		const CHAR* p2 = strstr(p1 + startLen, lpszEnd);
		if (NULL == p2)
			return std::string();

		size_t len = p2 - (p1 + startLen);
		if (len <= 0)
			return std::string();

		CHAR* lpszRet = new CHAR[len + 1];
		if (NULL == lpszRet)
			return std::string();

		memset(lpszRet, 0, sizeof(CHAR) * (len + 1));
		strncpy(lpszRet, p1 + startLen, len);
		strRet = lpszRet;
		delete[] lpszRet;

		return strRet;
	}

	static std::wstring GetBetweenStringW(const WCHAR* lpszStr, const WCHAR* lpszStart, const WCHAR* lpszEnd)
	{
		std::wstring strRet;

		if (NULL == lpszStr || NULL == lpszStart || NULL == lpszEnd)
			return std::wstring();

		size_t startLen = wcslen(lpszStart);

		const WCHAR* p1 = wcsstr(lpszStr, lpszStart);
		if (NULL == p1)
			return std::wstring();

		const WCHAR* p2 = wcsstr(p1 + startLen, lpszEnd);
		if (NULL == p2)
			return std::wstring();

		size_t len = p2 - (p1 + startLen);
		if (len <= 0)
			return std::wstring();

		WCHAR* lpszRet = new WCHAR[len + 1];
		if (NULL == lpszRet)
			return std::wstring();

		memset(lpszRet, 0, sizeof(WCHAR) * (len + 1));
		wcsncpy(lpszRet, p1 + startLen, len);
		strRet = lpszRet;
		delete[] lpszRet;

		return strRet;
	}

	static std::wstring AnsiToUnicode(const std::string& strAnsi)
	{
		std::wstring strUnicode;

		WCHAR* lpszUnicode = AnsiToUnicode(strAnsi.c_str());
		if (lpszUnicode != NULL)
		{
			strUnicode = lpszUnicode;
			delete[] lpszUnicode;
		}

		return strUnicode;
	}

	static std::string UnicodeToAnsi(const std::wstring& strUnicode)
	{
		std::string strAnsi;

		CHAR* lpszAnsi = UnicodeToAnsi(strUnicode.c_str());
		if (lpszAnsi != NULL)
		{
			strAnsi = lpszAnsi;
			delete[] lpszAnsi;
		}

		return strAnsi;
	}

	static std::string AnsiToUtf8(const std::string& strAnsi)
	{
		std::string strUtf8;

		CHAR* lpszUtf8 = AnsiToUtf8(strAnsi.c_str());
		if (lpszUtf8 != NULL)
		{
			strUtf8 = lpszUtf8;
			delete[] lpszUtf8;
		}

		return strUtf8;
	}

	static std::string Utf8ToAnsi(const std::string& strUtf8)
	{
		std::string strAnsi;

		CHAR* lpszAnsi = Utf8ToAnsi(strUtf8.c_str());
		if (lpszAnsi != NULL)
		{
			strAnsi = lpszAnsi;
			delete[] lpszAnsi;
		}

		return strAnsi;
	}

	static std::string UnicodeToUtf8(const std::wstring& strUnicode)
	{
		std::string strUtf8;

		CHAR* lpszUtf8 = UnicodeToUtf8(strUnicode.c_str());
		if (lpszUtf8 != NULL)
		{
			strUtf8 = lpszUtf8;
			delete[] lpszUtf8;
		}

		return strUtf8;
	}

	static std::wstring Utf8ToUnicode(const std::string& strUtf8)
	{
		std::wstring strUnicode;

		WCHAR* lpszUnicode = Utf8ToUnicode(strUtf8.c_str());
		if (lpszUnicode != NULL)
		{
			strUnicode = lpszUnicode;
			delete[] lpszUnicode;
		}

		return strUnicode;
	}

	static WCHAR* AnsiToUnicode(const CHAR* lpszStr)
	{
		WCHAR* lpszUnicode;
		int nLen;

		if (NULL == lpszStr)
			return NULL;

		nLen = ::MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, NULL, 0);
		if (0 == nLen)
			return NULL;

		lpszUnicode = new WCHAR[nLen + 1];
		if (NULL == lpszUnicode)
			return NULL;

		memset(lpszUnicode, 0, sizeof(WCHAR) * (nLen + 1));
		nLen = ::MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, lpszUnicode, nLen);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		return lpszUnicode;
	}

	static CHAR* UnicodeToAnsi(const WCHAR* lpszStr)
	{
		CHAR* lpszAnsi;
		int nLen;

		if (NULL == lpszStr)
			return NULL;

		nLen = ::WideCharToMultiByte(CP_ACP, 0, lpszStr, -1, NULL, 0, NULL, NULL);
		if (0 == nLen)
			return NULL;

		lpszAnsi = new CHAR[nLen + 1];
		if (NULL == lpszAnsi)
			return NULL;

		memset(lpszAnsi, 0, nLen + 1);
		nLen = ::WideCharToMultiByte(CP_ACP, 0, lpszStr, -1, lpszAnsi, nLen, NULL, NULL);
		if (0 == nLen)
		{
			delete[] lpszAnsi;
			return NULL;
		}

		return lpszAnsi;
	}

	static CHAR* AnsiToUtf8(const CHAR* lpszStr)
	{
		WCHAR* lpszUnicode;
		CHAR* lpszUtf8;
		int nLen;

		if (NULL == lpszStr)
			return NULL;

		nLen = ::MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, NULL, NULL);
		if (0 == nLen)
			return NULL;

		lpszUnicode = new WCHAR[nLen + 1];
		if (NULL == lpszUnicode)
			return NULL;

		memset(lpszUnicode, 0, sizeof(WCHAR) * (nLen + 1));
		nLen = ::MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, lpszUnicode, nLen);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		nLen = ::WideCharToMultiByte(CP_UTF8, 0, lpszUnicode, -1, NULL, 0, NULL, NULL);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		lpszUtf8 = new CHAR[nLen + 1];
		if (NULL == lpszUtf8)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		memset(lpszUtf8, 0, nLen + 1);
		nLen = ::WideCharToMultiByte(CP_UTF8, 0, lpszUnicode, -1, lpszUtf8, nLen, NULL, NULL);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			delete[] lpszUtf8;
			return NULL;
		}

		delete[] lpszUnicode;

		return lpszUtf8;
	}

	static CHAR* Utf8ToAnsi(const CHAR* lpszStr)
	{
		WCHAR* lpszUnicode;
		CHAR* lpszAnsi;
		int nLen;

		if (NULL == lpszStr)
			return NULL;

		nLen = ::MultiByteToWideChar(CP_UTF8, 0, lpszStr, -1, NULL, NULL);
		if (0 == nLen)
			return NULL;

		lpszUnicode = new WCHAR[nLen + 1];
		if (NULL == lpszUnicode)
			return NULL;

		memset(lpszUnicode, 0, sizeof(WCHAR) * (nLen + 1));
		nLen = ::MultiByteToWideChar(CP_UTF8, 0, lpszStr, -1, lpszUnicode, nLen);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		nLen = ::WideCharToMultiByte(CP_ACP, 0, lpszUnicode, -1, NULL, 0, NULL, NULL);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		lpszAnsi = new CHAR[nLen + 1];
		if (NULL == lpszAnsi)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		memset(lpszAnsi, 0, nLen + 1);
		nLen = ::WideCharToMultiByte(CP_ACP, 0, lpszUnicode, -1, lpszAnsi, nLen, NULL, NULL);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			delete[] lpszAnsi;
			return NULL;
		}

		delete[] lpszUnicode;

		return lpszAnsi;
	}

	static CHAR* UnicodeToUtf8(const WCHAR* lpszStr)
	{
		CHAR* lpszUtf8;
		int nLen;

		if (NULL == lpszStr)
			return NULL;

		nLen = ::WideCharToMultiByte(CP_UTF8, 0, lpszStr, -1, NULL, 0, NULL, NULL);
		if (0 == nLen)
			return NULL;

		lpszUtf8 = new CHAR[nLen + 1];
		if (NULL == lpszUtf8)
			return NULL;

		memset(lpszUtf8, 0, nLen + 1);
		nLen = ::WideCharToMultiByte(CP_UTF8, 0, lpszStr, -1, lpszUtf8, nLen, NULL, NULL);
		if (0 == nLen)
		{
			delete[] lpszUtf8;
			return NULL;
		}

		return lpszUtf8;
	}

	static WCHAR* Utf8ToUnicode(const CHAR* lpszStr)
	{
		WCHAR* lpszUnicode;
		int nLen;

		if (NULL == lpszStr)
			return NULL;

		nLen = ::MultiByteToWideChar(CP_UTF8, 0, lpszStr, -1, NULL, 0);
		if (0 == nLen)
			return NULL;

		lpszUnicode = new WCHAR[nLen + 1];
		if (NULL == lpszUnicode)
			return NULL;

		memset(lpszUnicode, 0, sizeof(WCHAR) * (nLen + 1));
		nLen = ::MultiByteToWideChar(CP_UTF8, 0, lpszStr, -1, lpszUnicode, nLen);
		if (0 == nLen)
		{
			delete[] lpszUnicode;
			return NULL;
		}

		return lpszUnicode;
	}
};



