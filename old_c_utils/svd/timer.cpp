#include <windows.h>

#define MTIMER_NUM 10
DWORD stamp[MTIMER_NUM];

void timerStart(unsigned int id)
{
	stamp[id] = GetTickCount();
}
unsigned int timerGet(unsigned int id)
{
	return GetTickCount() - stamp[id];
}
float timerGetS(unsigned int id)
{
	return float(GetTickCount() - stamp[id]) / 1000;
}
