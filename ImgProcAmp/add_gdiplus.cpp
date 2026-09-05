#include "pch.h"
#include "add_gdiplus.h"

bool GdiplusStart(ULONG_PTR* pGdiplusToken)
{
	GdiplusStartupInput gdiplusStartupInput;
	if (GdiplusStartup(pGdiplusToken, &gdiplusStartupInput, NULL) == Ok)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void GdiplusEnd(ULONG_PTR* pGdiplusToken)
{
	GdiplusShutdown(*pGdiplusToken);
}