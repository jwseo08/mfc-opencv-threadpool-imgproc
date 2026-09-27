#include "pch.h"
#include "ImgProcApi.h"
#include "ImgProcCore.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// cv::Mat raw 데이터 처리 결과 저장용
struct TImgRawInfo
{
	std::vector<unsigned char> vecData;
	int width = 0;
	int height = 0;
	int type = 0;
	int dataType = 0;
	size_t dataSize = 0;
};

// 기본 함수
int ProcessMat(const cv::Mat& matInput, cv::Mat& matOutput, const TImgProcOption& tOption)
{
	ImgProcCore imgCore;
	int rt = imgCore.ProcessMatImg(matInput, matOutput, tOption);
	if (rt != 0)
	{
		std::cerr << "process mat img fail=" << rt << "\n";
		matOutput = cv::Mat();
		return rt;
	}

	// 메모리 상의 데이터 연속성 보장
	if (!matOutput.isContinuous()) matOutput = matOutput.clone();
	return 0;
}

int ProcessMatRawImg(const unsigned char* inputData, 
	const int width, const int height, 
	const int type, const size_t step, 
	const TImgProcOption& tOption,
	ImageResultHandle* retPtr)
{
	if (inputData == nullptr ||
		width <= 0 ||
		height <= 0 ||
		type < 0 ||
		step == 0)
	{
		std::cerr << "input data invalid\n";
		return -99;
	}

	if (retPtr == nullptr)
	{
		std::cerr << "result handle invalid\n";
		return -98;
	}

	try
	{
		*retPtr = nullptr;

		cv::Mat matInput(height, width, type, (unsigned char*)inputData, step);
		cv::Mat matOutput;

		int rt = ProcessMat(matInput, matOutput, tOption);
		if (rt == 0)
		{
			// 결과 저장용 객체 생성 - 예외처리, 메모리 해제는 unique ptr로 처리
			// 예외 발생시 자동 해제, 사용자 사용 후 deleter 자동 호출
			auto imgRetInfo = std::make_unique<TImgRawInfo>();
			//TImgMatRawInfo* imgRetInfo = new TImgMatRawInfo;

			size_t imgRawDataSize = matOutput.total() * matOutput.elemSize();
			imgRetInfo->vecData.assign(matOutput.data, matOutput.data + imgRawDataSize);
			imgRetInfo->width = matOutput.cols;
			imgRetInfo->height = matOutput.rows;
			imgRetInfo->type = matOutput.type();
			imgRetInfo->dataType = 0;
			imgRetInfo->dataSize = imgRawDataSize;

			// 메모리 소유권을 사용자에게 전달하는 retPtr로 옮김
			*retPtr = (ImageResultHandle)imgRetInfo.release();
			return 0;
		}
		else
		{
			*retPtr = nullptr;

			std::cerr << "process mat img fail=" << rt << "\n";
			return rt;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "ProcessMatRawImg exception=" << e.what() << "\n";
		return -97;
	}
	catch (...)
	{
		std::cerr << "ProcessMatRawImg unknown exception\n";
		return -96;
	}
}

void GetProcessedMatRawImg(ImageResultHandle ptr, TImgRetInfo* imgRetInfo)
{
	if (ptr != nullptr && imgRetInfo != nullptr)
	{
		TImgRawInfo* matRawInfo = (TImgRawInfo*)ptr;

		imgRetInfo->data = matRawInfo->vecData.data();
		imgRetInfo->width = matRawInfo->width;
		imgRetInfo->height = matRawInfo->height;
		imgRetInfo->type = matRawInfo->type;
		imgRetInfo->dataType = matRawInfo->dataType;
		imgRetInfo->dataSize = matRawInfo->dataSize;
	}
	else if (imgRetInfo != nullptr)
	{
		imgRetInfo->data = nullptr;
		imgRetInfo->width = 0;
		imgRetInfo->height = 0;
		imgRetInfo->type = 0;
		imgRetInfo->dataType = 0;
		imgRetInfo->dataSize = 0;
	}
	else
	{
		std::cerr << "result ptr, result info nullprt";
	}
}

void ReleaseResult(ImageResultHandle ptr)
{
	delete (TImgRawInfo*)ptr;
}

int ProcessEncodedImg(const unsigned char* imgFileData, const size_t imgFileDataSize, 
	const TImgProcOption& tOption, const int outputEncodeType, ImageResultHandle* retPtr)
{
	if (imgFileData == nullptr || imgFileDataSize == 0)
	{
		std::cerr << "input image data invalid\n";
		return -1;
	}

	if (retPtr == nullptr)
	{
		std::cerr << "result pointer invalid\n";
		return -2;
	}

	*retPtr = nullptr;

	cv::Mat rawData(1, (int)imgFileDataSize, CV_8UC1, (unsigned char*)imgFileData);
	cv::Mat matInput = cv::imdecode(rawData, cv::IMREAD_COLOR);
	if (matInput.empty())
	{
		std::cerr << "decode image fail\n";
		return -3;
	}

	cv::Mat matOutput;
	int rt = ProcessMat(matInput, matOutput, tOption);
	if (rt != 0)
	{
		std::cerr << "process image fail=" << rt << "\n";
		return rt;
	}

	std::string ext = "";
	switch (outputEncodeType)
	{
	default:
	case FORMAT_JPG:
		ext = ".jpg";
		break;

	case FORMAT_PNG:
		ext = ".png";
		break;

	case FORMAT_BMP:
		ext = ".bmp";
		break;
	}

	std::vector<unsigned char> vEncoded;
	if (cv::imencode(ext, matOutput, vEncoded))
	{
		// 결과 저장용 객체 생성 - 예외처리, 메모리 해제는 unique ptr로 처리
		// 예외 발생시 자동 해제, 사용자 사용 후 deleter 자동 호출
		auto imgRetInfo = std::make_unique<TImgRawInfo>();
		//TImgMatRawInfo* imgRetInfo = new TImgMatRawInfo;
	
		size_t dataSize = vEncoded.size();

		imgRetInfo->vecData = std::move(vEncoded);
		imgRetInfo->width = 0;
		imgRetInfo->height = 0;
		imgRetInfo->type = 0;
		imgRetInfo->dataType = 1;
		imgRetInfo->dataSize = dataSize;

		// 메모리 소유권을 사용자에게 전달하는 retPtr로 옮김
		*retPtr = (ImageResultHandle)imgRetInfo.release();
		return 0;
	}
	else
	{
		return -2;
	}
}

int ProcessFileImg(const char* inputFileName, const char* outputFileName, const TImgProcOption& tOption)
{
	if (inputFileName == nullptr || strlen(inputFileName) == 0)
	{
		std::cerr << "input image filename invalid\n";
		return -1;
	}

	if (outputFileName == nullptr || strlen(outputFileName) == 0)
	{
		std::cerr << "output image filename invalid\n";
		return -2;
	}
	
	cv::Mat matInput = cv::imread(inputFileName, cv::IMREAD_COLOR);
	if (matInput.empty())
	{
		std::cerr << "input image file load fail\n";
		return -3;
	}

	cv::Mat matOutput;
	int rt = ProcessMat(matInput, matOutput, tOption);
	if (rt != 0)
	{
		std::cerr << "process image fail=" << rt << "\n";
		return rt;
	}

	if (cv::imwrite(outputFileName, matOutput))
	{
		return 0;
	}
	else
	{
		std::cerr << "output image file write fail\n";
		return -4;
	}
}
