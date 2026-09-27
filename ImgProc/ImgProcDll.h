#ifndef IMG_PROC_DLL_H
#define IMG_PROC_DLL_H

#include <Windows.h>
#include "ImgProcApi.h"

struct TImgProcDllFunc
{
	HMODULE hModule = NULL;

	using FuncProcessMatRawImg = int (*)(
		const unsigned char* inputData,
		const int width,
		const int height,
		const int type,
		const size_t step,
		const TImgProcOption& tOption,
		ImageResultHandle* retPtr);

	using FuncGetProcessedMatRawImg = void (*)(
		ImageResultHandle ptr,
		TImgRetInfo* imgRetInfo);

	using FuncReleaseResult = void (*)(
		ImageResultHandle ptr);

	using FuncProcessEncodedImg = int (*)(
		const unsigned char* imgFileData,
		const size_t imgFileDataSize,
		const TImgProcOption& tOption,
		const int outputEncodeType,
		ImageResultHandle* retPtr);

	using FuncProcessFileImg = int (*)(
		const char* inputFileName,
		const char* outputFileName,
		const TImgProcOption& tOption);

	FuncProcessMatRawImg ProcessMatRawImg = nullptr;
	FuncGetProcessedMatRawImg GetProcessedMatRawImg = nullptr;
	FuncReleaseResult ReleaseResult = nullptr;
	FuncProcessEncodedImg ProcessEncodedImg = nullptr;
	FuncProcessFileImg ProcessFileImg = nullptr;
};

inline void FreeImgProcDll(TImgProcDllFunc& dllFunc)
{
	::FreeLibrary(dllFunc.hModule);
	dllFunc.hModule = NULL;

	dllFunc.ProcessMatRawImg = nullptr;
	dllFunc.GetProcessedMatRawImg = nullptr;
	dllFunc.ReleaseResult = nullptr;
	dllFunc.ProcessEncodedImg = nullptr;
	dllFunc.ProcessFileImg = nullptr;
}

inline int LoadImgProcDll(const char* dllPathFile, TImgProcDllFunc& dllFunc)
{
	if (dllPathFile == nullptr)
	{
		return -1;
	}
	
	dllFunc.hModule = LoadLibrary(dllPathFile);
	if (dllFunc.hModule == NULL)
	{
		return -2;
	}

	int rt = 0;
	do
	{
		dllFunc.ProcessMatRawImg = reinterpret_cast<TImgProcDllFunc::FuncProcessMatRawImg>
			(::GetProcAddress(dllFunc.hModule, "ProcessMatRawImg"));
		if (dllFunc.ProcessMatRawImg == NULL)
		{
			rt = -3;
			break;
		}

		dllFunc.GetProcessedMatRawImg = reinterpret_cast<TImgProcDllFunc::FuncGetProcessedMatRawImg>
			(::GetProcAddress(dllFunc.hModule, "GetProcessedMatRawImg"));
		if (dllFunc.GetProcessedMatRawImg == NULL)
		{
			rt = -4;
			break;
		}

		dllFunc.ReleaseResult = reinterpret_cast<TImgProcDllFunc::FuncReleaseResult>
			(::GetProcAddress(dllFunc.hModule, "ReleaseResult"));
		if (dllFunc.ReleaseResult == NULL)
		{
			rt = -5;
			break;
		}

		dllFunc.ProcessEncodedImg = reinterpret_cast<TImgProcDllFunc::FuncProcessEncodedImg>
			(::GetProcAddress(dllFunc.hModule, "ProcessEncodedImg"));
		if (dllFunc.ProcessEncodedImg == NULL)
		{
			rt = -6;
			break;
		}

		dllFunc.ProcessFileImg = reinterpret_cast<TImgProcDllFunc::FuncProcessFileImg>
			(::GetProcAddress(dllFunc.hModule, "ProcessFileImg"));
		if (dllFunc.ProcessFileImg == NULL)
		{
			rt = -7;
			break;
		}

	} while (0);

	if (rt != 0)
	{
		FreeImgProcDll(dllFunc);
		return rt;
	}

	return 0;
}

// 이미지 처리 결과 메모리 해제 helper
struct ImgProcDllResultDeleter
{
	TImgProcDllFunc::FuncReleaseResult releaseResult = nullptr;

	void operator()(void* handle) noexcept
	{
		if (handle != nullptr && releaseResult != nullptr)
		{
			releaseResult(handle);
		}
	}
};

using ImgProcDllResultPtr = std::unique_ptr<void, ImgProcDllResultDeleter>;

// 이미지 처리 결과 cv::Mat raw 데이터와 정보를 가져오고 결과 메모리 자동 해제 연결
inline ImgProcDllResultPtr MakeImgProcDllResultPtr(const TImgProcDllFunc& dllFunc, ImageResultHandle handle) noexcept
{
	return ImgProcDllResultPtr(handle, ImgProcDllResultDeleter{ dllFunc.ReleaseResult });
}

#endif

/*
ImageResultHandle rawHandle = nullptr;

int rt = dllFunc.ProcessMatRawImg(
	inputData,
	width,
	height,
	type,
	step,
	option,
	&rawHandle);

if (rt != 0)
	return false;

auto result = MakeImgProcDllResultPtr(dllFunc, rawHandle);

TImgRetInfo retInfo;
dllFunc.GetProcessedMatRawImg(result.get(), &retInfo);

// result가 범위를 벗어나면 dllFunc.ReleaseResult()가 자동 호출됨
*/