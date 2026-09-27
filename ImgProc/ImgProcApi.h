#ifndef  IMG_PROC_H
#define IMG_PROC_H

#include <memory>
#include <cstddef>

#define EXPORT_DLL 

#ifdef EXPORT_DLL
#define IMGPROC_API __declspec(dllexport)
#else
#define IMGPROC_API __declspec(dllimport)
#endif

// 처리 결과 전달용
using ImageResultHandle = void*;

// 처리 결과 전달용 - 처리 결과 이미지 정보
struct TImgRetInfo
{
	const unsigned char* data = nullptr;
	int width = 0;
	int height = 0;
	int type = 0;
	int dataType = 0;
	size_t dataSize = 0;
};

// 이미지 처리작업 옵션
// 이미지 처리 옵션
struct TImgProcOption
{
	int nMaxWidth = 1600;               // 최대 가로 크기
	int nMaxHeight = 1600;              // 최대 세로 크기

	int nBilateralDiameter = 7;         // 양방향 필터 이웃 지름
	double dBilateralSigmaColor = 50.0; // 양방향 필터 색 차이 범위
	double dBilateralSigmaSpace = 50.0; // 양방향 필터 공간 거리 범위

	double dClaheClipLimit = 2.0;       // clahe 대비 증폭 제한값
	int nClaheTileSize = 8;             // clahe 가로세로 타일 개수
	double dGamma = 0.0;                // 감마값 - 0 이하면 자동 계산

	double dCannyThreshold1 = 50.0;     // canny 하위 임계값
	double dCannyThreshold2 = 150.0;    // canny 상위 임계값
	double dMinDocumentAreaRatio = 0.15;// 문서 최소 면적 비율 - 전체 영상 기준

	int nShadowKernelSize = 21;         // 그림자 제거 커널 크기 - 홀수 보정
	int nAdaptiveBlockSize = 31;        // 이진화 블록 크기 - 홀수 보정
	double dAdaptiveC = 7.0;            // 이진화 보정 상수 - 주변 가중 평균 기반으로 변경
	double dSharpenAmount = 1.0;        // unsharp 마스크 강도 - 0.0~3.0 제한
};

// 출력 이미지 인코딩 형식
enum OutputImgFormat
{
	FORMAT_JPG = 0,
	FORMAT_PNG = 1,
	FORMAT_BMP = 2
};

extern "C"
{
	// cv::Mat의 raw 데이터를 입력받아서 이미지 처리
	IMGPROC_API int ProcessMatRawImg(const unsigned char* inputData, 
		const int width, const int height, 
		const int type, const size_t step, 
		const TImgProcOption& tOption,
		ImageResultHandle* retPtr);

	// 위 함수 결과 가져오기 - cv::Mat의 raw 데이터
	IMGPROC_API void GetProcessedMatRawImg(ImageResultHandle ptr, TImgRetInfo* imgRetInfo);

	// 위 함수 결과 메모리 해제
	IMGPROC_API void ReleaseResult(ImageResultHandle ptr);

	/*
	IMGPROC_API int ProcessMatRawImg(const unsigned char* imgData, const int width, const int height,
		const int type, const size_t step, 
		unsigned char** outputData, int* outputWidth, int* outputHeight, int* outputType);
	*/
	
	IMGPROC_API int ProcessEncodedImg(const unsigned char* imgFileData, const size_t imgFileDataSize,
		const TImgProcOption& tOption, const int outputEncodeType, ImageResultHandle* retPtr);
	
	IMGPROC_API int ProcessFileImg(const char* inputFileName, const char* outputFileName, const TImgProcOption& tOption);
}

// 이미지 처리 결과 메모리 해제 helper
struct ImageResultDeleter
{
	void operator()(void* handle) noexcept
	{
		if (handle != nullptr) ReleaseResult(handle);
	}
};

using ImageResultPtr = std::unique_ptr<void, ImageResultDeleter>;

#endif // ! img_proc_h


// 사용 예
/*
ImageResultHandle rawHandle = nullptr;

int rt = ProcessMatRawImg(
	inputData,
	width,
	height,
	type,
	step,
	&rawHandle);

if (rt != 0) return false;

ImageResultPtr result(rawHandle);
TImgRetInfo retInfo;

GetProcessedMatRawImg(result.get(), &retInfo);
*/