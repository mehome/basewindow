#pragma once
#ifdef NODE_EXPORTS
#define NodeDeclear __declspec(dllexport) 
#else
#define NodeDeclear __declspec(dllimport) 
#endif