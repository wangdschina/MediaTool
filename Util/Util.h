#ifdef UTIL_EXPORTS
#define UTIL_API __declspec(dllexport)
#else
#define UTIL_API __declspec(dllimport)
#endif

#include <winsock2.h>
#include <shobjidl.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlimage.h>

#include <ImageHlp.h>
#include <random>

#include "ByteStream.h"
#include "StringUtil.h"
#include "Path.h"

#include "BaseFunc.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment (lib, "crypt32.lib")
#pragma comment(lib, "winmm.lib")
#pragma warning(disable:4786)
