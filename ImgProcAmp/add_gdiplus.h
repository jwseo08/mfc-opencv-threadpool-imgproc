#pragma once

#include "gdiplus.h"

using namespace Gdiplus;

#pragma comment (lib, "gdiplus.lib")

bool GdiplusStart(ULONG_PTR* pGdiplusToken);
void GdiplusEnd(ULONG_PTR* pGdiplusToken);