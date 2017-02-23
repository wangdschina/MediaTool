// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <strsafe.h>


// TODO: reference additional headers your program requires here
#include "StringUtil.h"
#include "Def.h"

#include "Util.h"
#include "AVILib.h"

#include "UIlib.h"
using namespace DuiLib;

#include "hi_config.h"
#include "hi_h264api.h"

#include "SDL.h"
#include "Direct3d.h"
#include "CWndUI.h"

typedef std::basic_string<TCHAR> tString;

#define USE(FEATURE) (defined USE_##FEATURE  && USE_##FEATURE)
#define ENABLE(FEATURE) (defined ENABLE_##FEATURE  && ENABLE_##FEATURE)

#define USE_ZIP_SKIN 1
#define USE_EMBEDED_RESOURCE 1

#include <sstream>
#include <io.h>
#include <comutil.h>

#include <shobjidl.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlimage.h>

#include <shellapi.h>

#include <thread>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "winmm.lib")
#pragma warning(disable:4786)