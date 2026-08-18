#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include "utils.h"
#include <wrl.h>

using Microsoft::WRL::ComPtr;

//#define D3DCOMPILE_DEBUG 1
#define CHECK_FAIL(fn) if(!fn) { return false; } 
#define BREAK_IF_FAIL(fn, msg)  if(!fn) { std::cerr << msg; break; } 

#define LOGHR(hr, msg) \
std::string fullMsg = msg + Utils::HrToAString(hr); \
MessageBoxA(0, fullMsg.c_str(), "Error", MB_OK);

#define LOG_HR_AND_RETURN_FAIL(hr, msg) \
LOGHR(hr, msg); \
return false;

#define PROMPTFAILHR(hr, msg) \
if(FAILED(hr)) \
{\
LOG_HR_AND_RETURN_FAIL(hr, msg);\
}\
// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

// 0 = off
// 1 = 60 fps
// 2 = 30 fps
#define VSYNC 0 

#define PI 3.14159

#define DIRECTXM_VECTOR_SIZE 16