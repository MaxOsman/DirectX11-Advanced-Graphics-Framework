#pragma once
#include <string>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <sstream>

class Debug
{
public:
	void Print(const char* s);
	void Print(int i);
	void Print(float f);
};