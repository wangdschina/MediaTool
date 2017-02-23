#ifndef TINYCONFIG_INCLUDED
#define TINYCONFIG_INCLUDED

#ifdef TINYXML_EXPORTS
#define TINYXML_API __declspec(dllexport)
#else
#define TINYXML_API __declspec(dllimport)
#endif

#endif // TINYCONFIG_INCLUDED