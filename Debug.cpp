#include "Debug.h"

void Debug::Print(const char* s)
{
	//char* message;
	//sprintf(message, s, NULL);
	OutputDebugStringA(s);
	OutputDebugStringA("\n");
}

//forums.codeguru.com/showthread.php?497457-How-to-use-OutputDebugString
void Debug::Print(int i)
{
	TCHAR s[16];
	swprintf(s, _T("%d"), i);
	OutputDebugString(s);
	OutputDebugStringA("\n");
}

//stackoverflow.com/questions/16258493/output-float-to-debug
void Debug::Print(float f)
{
	std::ostringstream ss;
	ss << f;
	OutputDebugStringA(ss.str().c_str());
	OutputDebugStringA("\n");
}