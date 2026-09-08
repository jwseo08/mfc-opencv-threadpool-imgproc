#include "pch.h"
#include "CImgProc.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "CommonUtil.h"

// 이미지 처리 - 파이프라인 구성
bool CImgProc::Process(const CString& csImageFileName, TImgProcResult& tResult, const TImgProcOption& tOption)
{
	// 작업 결과 초기화
	tResult = TImgProcResult();

	try
	{
		cv::Mat matInput;
		if (!LoadImageFile(csImageFileName, matInput))
		{
			tResult.csErrorMessage = _T("이미지 파일을 읽을 수 없습니다.");
			return false;
		}

		// 각파이프라인 단계 별 출력 - 다음 단계의 입력으로 전달
		cv::Mat matResized;       // 크기 조정, 채널 형식 변경 이미지
		cv::Mat matDenoised;      // 노이즈 제거 이미지
		cv::Mat matContrast;      // clahe 기반 부분 영역 대비 보정, 전체 밝기 보정 이미지
		cv::Mat matPerspective;   // 원근 보정 이미지 - 원근 보정이 실패한 경우 matContrast의 복사본
		cv::Mat matShadowRemoved; // 불균일 배경 조명 보정, 그림자 완화 보정 이미지

		// 크기 조정, 채널 형식 변경
		if (!ResizeAndConvertColor(matInput, matResized, tOption))
			throw std::runtime_error("리사이즈 및 색 공간 변환 실패");

		// 노이즈 감소
		if (!RemoveNoise(matResized, matDenoised, tOption))
			throw std::runtime_error("노이즈 제거 실패");

		// clahe 기반 부분 대비 보정, 전체 밝기 감마 보정
		if (!ApplyClaheAndGamma(matDenoised, matContrast, tOption))
			throw std::runtime_error("clahe 및 감마 보정 실패");

		// 문서 영역 찾기 - 경계선, 모서리 꼭지점 찾음
		// 검출되지 않은 경우 입력 이미지를 그대로 다음 단계로 전달
		if (DetectDocumentContour(matContrast, tResult.matCanny, tResult.vecDocumentCorners, tOption))
		{
			// 검출 성공하면 원근 보정 적용
			tResult.bPerspectiveApplied = CorrectPerspective(matContrast, tResult.vecDocumentCorners, matPerspective);
		}

		// 원근 보정 실패하면 대비 밝기 보정 이미지을 다음 단계로 넘김
		if (!tResult.bPerspectiveApplied) matPerspective = matContrast.clone();

		// 그림자 보정
		if (!RemoveShadow(matPerspective, matShadowRemoved, tOption))
		{
			throw std::runtime_error("그림자 제거 실패");
		}

		// 이진화
		if (!BinarizeAndSharpen(matShadowRemoved, tResult.matResult, tOption))
		{
			throw std::runtime_error("이진화 및 선명화 실패");
		}

		tResult.bSuccess = true;
		return true;
	}
	catch (const cv::Exception& e)
	{
		tResult.csErrorMessage.Format(_T("OpenCV 오류: %S"), e.what());
	}
	catch (const std::exception& e)
	{
		tResult.csErrorMessage.Format(_T("이미지 처리 오류: %S"), e.what());
	}

	tResult.matResult.release();
	return false;
}

// 이미지 처리와 결과 이미지 저장 - 파이프라인 구성 - process 작업에 파일 저장 추가
bool CImgProc::ProcessAndSave(
	const CString& csImageFileName,
	const CString& csSaveFileName,
	TImgProcResult& tResult,
	const TImgProcOption& tOption)
{
	// 작업 결과 초기화
	tResult = TImgProcResult();

	try
	{
		cv::Mat matInput;
		if (!LoadImageFile(csImageFileName, matInput))
		{
			tResult.csErrorMessage = _T("이미지 파일을 읽을 수 없습니다.");
			return false;
		}

		cv::Mat matResized;       
		cv::Mat matDenoised;      
		cv::Mat matContrast;      
		cv::Mat matPerspective;   
		cv::Mat matShadowRemoved;

		if (!ResizeAndConvertColor(matInput, matResized, tOption))
			throw std::runtime_error("리사이즈 및 색 공간 변환 실패");

		if (!RemoveNoise(matResized, matDenoised, tOption))
			throw std::runtime_error("노이즈 제거 실패");

		if (!ApplyClaheAndGamma(matDenoised, matContrast, tOption))
			throw std::runtime_error("CLAHE 및 감마 보정 실패");

		if (DetectDocumentContour(matContrast, tResult.matCanny, tResult.vecDocumentCorners, tOption))
		{
			tResult.bPerspectiveApplied = CorrectPerspective(matContrast, tResult.vecDocumentCorners, matPerspective);
		}

		if (!tResult.bPerspectiveApplied) matPerspective = matContrast.clone();

		if (!RemoveShadow(matPerspective, matShadowRemoved, tOption))
			throw std::runtime_error("그림자 제거 실패");

		if (!BinarizeAndSharpen(matShadowRemoved, tResult.matResult, tOption))
			throw std::runtime_error("이진화 및 선명화 실패");

		// 결과 이미지를 파일로 저장
		if (!cv::imwrite(csSaveFileName.GetString(), tResult.matResult))
			throw std::runtime_error("결과 파일 저장 실패");

		tResult.bSuccess = true;
		return true;
	}
	catch (const cv::Exception& e)
	{
		tResult.csErrorMessage.Format(_T("OpenCV 오류: %S"), e.what());
	}
	catch (const std::exception& e)
	{
		tResult.csErrorMessage.Format(_T("이미지 처리 오류: %S"), e.what());
	}

	tResult.matResult.release();
	return false;
}

bool CImgProc::ResizeAndConvertColor(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption)
{
	if (matSrc.empty() || tOption.nMaxWidth <= 0 || tOption.nMaxHeight <= 0) return false;

	// 채널 수가 다른 이미지를 BGR 3채널로 변환
	cv::Mat matBgr;
	switch (matSrc.channels())
	{
	case 1: // 1채널 grayscale 이미지
		cv::cvtColor(matSrc, matBgr, cv::COLOR_GRAY2BGR);
		break;
	case 3:
		matBgr = matSrc;
		break;
	case 4: // 4채널 알파채널 포함 이미지
		cv::cvtColor(matSrc, matBgr, cv::COLOR_BGRA2BGR);
		break;
	default:
		return false;
	}

	// 최대 크기보다 크면 크기 조정 - 가로세로 비율 유지
	const double dScaleX = static_cast<double>(tOption.nMaxWidth) / matBgr.cols;
	const double dScaleY = static_cast<double>(tOption.nMaxHeight) / matBgr.rows;
	const double dScale = std::min(1.0, std::min(dScaleX, dScaleY));

	if (dScale < 1.0)
	{
		// 최대 크기보다 큰 경우
		cv::resize(matBgr, matDst, cv::Size(), dScale, dScale, cv::INTER_AREA);
	}
	else
	{
		// 최대 크기보다 작은 경우
		matDst = matBgr.clone();
	}

	if (!matDst.empty()) return true;
	else return false;
}

bool CImgProc::RemoveNoise(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption)
{
	if (matSrc.empty()) return false;

	// 양방향 필터 적용 - 윤곽선을 최대한 보존하면서 노이즈 감소
	const int nDiameter = std::max(1, tOption.nBilateralDiameter);
	cv::bilateralFilter(
		matSrc,
		matDst,
		nDiameter,
		std::max(1.0, tOption.dBilateralSigmaColor),
		std::max(1.0, tOption.dBilateralSigmaSpace));

	if (!matDst.empty()) return true;
	else return false;
}

bool CImgProc::ApplyClaheAndGamma(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption)
{
	if (matSrc.empty() || matSrc.channels() != 3) return false;

	// lab 컬러로 변환 - 색상과 밝기 분리
	// 밝기 채널만 보정해서 색상에 대한 영향 최소화
	cv::Mat matLab;
	cv::cvtColor(matSrc, matLab, cv::COLOR_BGR2Lab);

	std::vector<cv::Mat> vecChannels;
	cv::split(matLab, vecChannels);

	// clahe 파라미터 설정 - clahe 작업에 사용하는 타일 그리드 개수 최소값을 2로 설정
	const int nTileSize = std::max(2, tOption.nClaheTileSize);

	// clahe 생성, lab 컬러의 L 채널에 clahe 적용
	// 밝기 채널에 clahe 적용 - 부분 밝기 대비 보정 
	cv::Ptr<cv::CLAHE> pClahe = cv::createCLAHE(std::max(0.1, tOption.dClaheClipLimit), cv::Size(nTileSize, nTileSize));
	pClahe->apply(vecChannels[0], vecChannels[0]);

	// clahe 적용이 끝나면 색상 채널과 밝기 채널을 합치고 BGR 형식으로 변환
	cv::merge(vecChannels, matLab);
	cv::Mat matClahe;
	cv::cvtColor(matLab, matClahe, cv::COLOR_Lab2BGR);

	// 전체 밝기 보정을 위한 감마값 설정
	// 명시적인 감마값이 없으면 현재 영상의 평균 밝기를 기반으로 계산
	double dGamma = tOption.dGamma;
	if (dGamma <= 0.0) dGamma = CalculateAutoGamma(matClahe);
	dGamma = std::clamp(dGamma, 0.35, 3.0);

	// look up table 작성 - 각 픽셀에 대한 pow 반복 작업 대체
	cv::Mat matLut(1, 256, CV_8U);
	for (int i = 0; i < 256; ++i)
	{
		matLut.at<uchar>(i) = cv::saturate_cast<uchar>(std::pow(i / 255.0, dGamma) * 255.0);
	}

	// 감마값 적용으로 전체 밝기 보정 - look up table 적용
	cv::LUT(matClahe, matLut, matDst);
	
	if (!matDst.empty()) return true;
	else return false;
}

bool CImgProc::DetectDocumentContour(
	const cv::Mat& matSrc,
	cv::Mat& matCanny,
	std::vector<cv::Point2f>& vecCorners,
	const TImgProcOption& tOption)
{
	// 결과값 초기화
	vecCorners.clear();
	matCanny.release();

	if (matSrc.empty()) return false;

	// grayscale 변환 - canny 경계선 검출을 위한 전처리
	cv::Mat matGray;
	if (matSrc.channels() == 3) cv::cvtColor(matSrc, matGray, cv::COLOR_BGR2GRAY);
	else matGray = matSrc;

	// blur 적용 - canny 경계선 검출을 위한 전처리, 미세 노이즈 제거
	cv::GaussianBlur(matGray, matGray, cv::Size(5, 5), 0.0);
	
	//canny 경계선 검출 - 밝기 변화가 큰 문서 외곽선, 글자 경계선 검출
	cv::Canny(
		matGray,
		matCanny,
		std::max(0.0, tOption.dCannyThreshold1),
		std::max(tOption.dCannyThreshold1 + 1.0, tOption.dCannyThreshold2));

	// 끊어진 경계선을 닫힘 연산으로 이어줌 - 문서 외곽선 검출을 위한 보정
	const cv::Mat matKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::morphologyEx(matCanny, matCanny, cv::MORPH_CLOSE, matKernel, cv::Point(-1, -1), 2);

	std::vector<std::vector<cv::Point>> vecContours;
	cv::findContours(matCanny.clone(), vecContours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	// 너무 작은 영역이 문서로 오검출되지 않도록 영역 크기 최소값 지정
	const double dImageArea = static_cast<double>(matSrc.cols) * matSrc.rows;
	const double dMinArea = dImageArea * std::clamp(tOption.dMinDocumentAreaRatio, 0.01, 0.95);

	// 조건을 만족하는 사각형 영역 중에서 가장 크기가 큰 것을 문서 경계선으로 지정
	double dBestArea = 0.0;
	std::vector<cv::Point> vecBestCorners;

	for (const auto& vecContour : vecContours)
	{
		const double dArea = std::fabs(cv::contourArea(vecContour));
		if (dArea < dMinArea || dArea <= dBestArea) continue;

		// 꼭지점이 4개인지 검사 - 정밀도는 윤곽선 둘레 2%
		const double dPerimeter = cv::arcLength(vecContour, true);
		std::vector<cv::Point> vecApprox;
		cv::approxPolyDP(vecContour, vecApprox, dPerimeter * 0.02, true);

		// 꼭지점이 4개이고 볼록 사각형인 경우 문서 후보로 지정
		if (vecApprox.size() == 4 && cv::isContourConvex(vecApprox))
		{
			dBestArea = dArea;
			vecBestCorners = std::move(vecApprox);
		}
	}

	if (vecBestCorners.size() != 4) return false;

	// 찾은 꼭지점을 순서대로 정렬 - 좌상, 우상, 우하, 좌하 순서
	vecCorners = OrderCorners(vecBestCorners);

	if (vecCorners.size() == 4) return true;
	else return false;
}

bool CImgProc::CorrectPerspective(const cv::Mat& matSrc, const std::vector<cv::Point2f>& vecCorners, cv::Mat& matDst)
{
	if (matSrc.empty() || vecCorners.size() != 4) return false;

	// 서로 마주 보는 두 변 중 긴 길이를 보정 결과 사각형의 너비와 높이로 사용
	const double dTopWidth = cv::norm(vecCorners[1] - vecCorners[0]);
	const double dBottomWidth = cv::norm(vecCorners[2] - vecCorners[3]);
	const double dLeftHeight = cv::norm(vecCorners[3] - vecCorners[0]);
	const double dRightHeight = cv::norm(vecCorners[2] - vecCorners[1]);

	const int nWidth = static_cast<int>(std::round(std::max(dTopWidth, dBottomWidth)));
	const int nHeight = static_cast<int>(std::round(std::max(dLeftHeight, dRightHeight)));
	if (nWidth < 2 || nHeight < 2) return false;

	// 검출된 네 점에 대응하는 보정된 직사각형 좌표 설정 - 보정 목표 좌표
	const std::vector<cv::Point2f> vecDestination =
	{
		cv::Point2f(0.0f, 0.0f),
		cv::Point2f(static_cast<float>(nWidth - 1), 0.0f),
		cv::Point2f(static_cast<float>(nWidth - 1), static_cast<float>(nHeight - 1)),
		cv::Point2f(0.0f, static_cast<float>(nHeight - 1))
	};

	// 보정 목표 좌표로 변환하기 위한 변환 행렬 계산 - 원근 변환
	const cv::Mat matTransform = cv::getPerspectiveTransform(vecCorners, vecDestination);
	
	// 워핑 적용 - 기울어진 이미지 보정
	cv::warpPerspective(
		matSrc,
		matDst,
		matTransform,
		cv::Size(nWidth, nHeight),
		cv::INTER_CUBIC,
		cv::BORDER_REPLICATE);

	if (!matDst.empty()) return true;
	else return false;
}

bool CImgProc::RemoveShadow(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption)
{
	if (matSrc.empty()) return false;

	// 컬러 이미지 채널 분리 - 채널 별로 정규화 적용하고 다시 합침
	std::vector<cv::Mat> vecChannels;
	if (matSrc.channels() == 1) vecChannels.push_back(matSrc);
	else cv::split(matSrc, vecChannels);

	// morphology 연산에서 사용하는 그림자 제거 커널 생성
	// 사각형 영역 형태, 중심 anchor를 위한 홀수 크기로 지정 - 글자보다 큰 영역을 처리하도록 설정 - 주의
	const int nKernelSize = MakeOdd(tOption.nShadowKernelSize, 3);
	const cv::Mat matKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(nKernelSize, nKernelSize));

	// 각 채널 별로 처리
	for (cv::Mat& matChannel : vecChannels)
	{
		// 크기가 큰 커널을 사용해서 닫힘 연산 적용
		// 문자 처럼 작은 어두운 영역의 영향 줄임, 완만하게 변화하는 배경 조명 분포 추정
		// 결과물인 matBackground은 전체적인 조명과 그림자 분포를 추정하는 영상
		cv::Mat matBackground;
		cv::morphologyEx(matChannel, matBackground, cv::MORPH_CLOSE, matKernel);

		// 처리 대상 채널과 background 영상 데이터를 32bit float 로 변환
		// 후속 작업 정밀도 유지를 위해서 변환 - 후속 작업에서 채널 픽셀값을 배경 밝기로 나눔
		cv::Mat matFloatChannel;
		cv::Mat matFloatBackground;
		matChannel.convertTo(matFloatChannel, CV_32F);
		matBackground.convertTo(matFloatBackground, CV_32F);
		matFloatBackground += 1.0f; // divide by zero 방지

		// 픽셀을 배경 밝기로 나눔 - 조명 변화와 그림자 완화
		cv::divide(matFloatChannel, matFloatBackground, matFloatChannel, 255.0);

		// 처리가 끝나면 해당 채널을 8비트 영상 형식으로 변환
		matFloatChannel.convertTo(matChannel, CV_8U);
	}

	// 모든 채널 처리가 끝나면 채널 합침
	if (vecChannels.size() == 1) matDst = vecChannels[0];
	else cv::merge(vecChannels, matDst);

	if (!matDst.empty()) return true;
	else return false;
}

bool CImgProc::BinarizeAndSharpen(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption)
{
	if (matSrc.empty()) return false;

	cv::Mat matGray;

	// 3채널 컬러 이미지는 grayscale로 변환 
	if (matSrc.channels() == 3) cv::cvtColor(matSrc, matGray, cv::COLOR_BGR2GRAY);
	else matGray = matSrc.clone();

	// blur 적용 이미지 생성 - unsharp 마스크 적용을 위한 전처리
	cv::Mat matBlurred;
	cv::GaussianBlur(matGray, matBlurred, cv::Size(0, 0), 1.2);

	// unsharp 마스크 적용 - grayscale 이미지와 blur 적용 이미지 차이 이용 - 글자 경계 강조 
	cv::Mat matSharpened;
	const double dAmount = std::clamp(tOption.dSharpenAmount, 0.0, 3.0);
	cv::addWeighted(matGray, 1.0 + dAmount, matBlurred, -dAmount, 0.0, matSharpened);

	// 이진화
	// 조명이 균일하지 않더라도 처리가 가능하도록 위치별 주변 밝기를 기준으로 임계값 설정
	const int nBlockSize = MakeOdd(tOption.nAdaptiveBlockSize, 3);
	cv::adaptiveThreshold(
		matSharpened,
		matDst,
		255,
		cv::ADAPTIVE_THRESH_GAUSSIAN_C,
		cv::THRESH_BINARY,
		nBlockSize,
		tOption.dAdaptiveC);

	if (!matDst.empty()) return true;
	else return false;
}

bool CImgProc::LoadImageFile(const CString& csImageFileName, cv::Mat& matImage)
{
	matImage.release();
	if (csImageFileName.IsEmpty()) return false;

	std::vector<uchar> vecData;
	int rt = ReadFileToVecBuf(csImageFileName.GetString(), vecData);
	if (rt == 0)
	{
		matImage = cv::imdecode(vecData, cv::IMREAD_UNCHANGED);
		if (matImage.empty())
		{
			std::cerr << "image decode fail \n";
			return false;
		}
	}
	else
	{
		std::cerr << "image load fail=" << rt << "\n";
		return false;
	}

	return true;
}

double CImgProc::CalculateAutoGamma(const cv::Mat& matSrc)
{
	cv::Mat matGray;
	if (matSrc.channels() == 3) cv::cvtColor(matSrc, matGray, cv::COLOR_BGR2GRAY);
	else matGray = matSrc;

	// 감마 보정 후 평균 밝기가 0.5가 되도록 감마값 계산
    // log(0)를 방지하기 위해서 평균 밝기를 0.01~0.99 범위로 제한
	const double dMean = std::clamp(cv::mean(matGray)[0] / 255.0, 0.01, 0.99);
	return std::log(0.5) / std::log(dMean);
}

std::vector<cv::Point2f> CImgProc::OrderCorners(const std::vector<cv::Point>& vecCorners)
{
	if (vecCorners.size() != 4) return {};

	// x + y 는 좌상 우하 구분에 사용
	// y - x 는 우상 좌하 구분에 사용
	std::vector<cv::Point2f> vecOrdered(4);
	double dMinSum = std::numeric_limits<double>::max();
	double dMaxSum = std::numeric_limits<double>::lowest();
	double dMinDiff = std::numeric_limits<double>::max();
	double dMaxDiff = std::numeric_limits<double>::lowest();

	for (const cv::Point& pt : vecCorners)
	{
		const double dSum = static_cast<double>(pt.x) + pt.y;
		const double dDiff = static_cast<double>(pt.y) - pt.x;

		if (dSum < dMinSum)
		{
			dMinSum = dSum;
			vecOrdered[0] = cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y));
		}
		if (dDiff < dMinDiff)
		{
			dMinDiff = dDiff;
			vecOrdered[1] = cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y));
		}
		if (dSum > dMaxSum)
		{
			dMaxSum = dSum;
			vecOrdered[2] = cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y));
		}
		if (dDiff > dMaxDiff)
		{
			dMaxDiff = dDiff;
			vecOrdered[3] = cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y));
		}
	}

	return vecOrdered;
}

int CImgProc::MakeOdd(int nValue, int nMinimum)
{
	// morphology 커널에 사용하기 위해서 anchor 중심 픽셀이 있는 홀수 크기로 지정
	int nResult = std::max(nValue, nMinimum);
	if ((nResult % 2) == 0) ++nResult;
	return nResult;
}
