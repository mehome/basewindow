#pragma once

#ifdef NODE2D_EXPORTS
#define Node2DDeclear __declspec(dllexport) 
#else
#define Node2DDeclear __declspec(dllimport) 
#endif
