#include "pch.h"
#include "CImgProc.h"

#include <algorithm>
#include <cmath>
#include <limits>

// 이미지 처리 - 파이프라인 구성
bool CImgProc::Process(const CString& csImageFileName, TImgProcResult& tResult, const TImgProcOption& tOption) const
{
	// 이전 호출의 영상/오류 상태가 남지 않도록 출력 구조체를 기본값으로 재설정한다.
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

// 이미지 처리와 결과 이미지 저장 - 파이프라인 구성
bool CImgProc::ProcessAndSave(
	const CString& csImageFileName,
	const CString& csSaveFileName,
	TImgProcResult& tResult,
	const TImgProcOption& tOption) const
{
	// 작업 상태 초기화
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

bool CImgProc::ResizeAndConvertColor(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const
{
	if (matSrc.empty() || tOption.nMaxWidth <= 0 || tOption.nMaxHeight <= 0) return false;

	// 이후 단계가 항상 동일한 채널 형식을 받도록 입력을 BGR 3채널로 통일한다.
	cv::Mat matBgr;
	switch (matSrc.channels())
	{
	case 1:
		cv::cvtColor(matSrc, matBgr, cv::COLOR_GRAY2BGR);
		break;
	case 3:
		matBgr = matSrc;
		break;
	case 4:
		cv::cvtColor(matSrc, matBgr, cv::COLOR_BGRA2BGR);
		break;
	default:
		return false;
	}

	// 가로/세로 제한 중 더 엄격한 배율을 선택하고 1.0을 상한으로 두어 확대를 막는다.
	const double dScaleX = static_cast<double>(tOption.nMaxWidth) / matBgr.cols;
	const double dScaleY = static_cast<double>(tOption.nMaxHeight) / matBgr.rows;
	const double dScale = std::min(1.0, std::min(dScaleX, dScaleY));

	if (dScale < 1.0)
	{
		cv::resize(matBgr, matDst, cv::Size(), dScale, dScale, cv::INTER_AREA);
	}
	else
	{
		matDst = matBgr.clone();
	}

	return !matDst.empty();
}

bool CImgProc::RemoveNoise(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const
{
	if (matSrc.empty()) return false;

	// 양방향 필터는 공간 거리와 색 차이를 함께 고려해 글자 경계의 번짐을 줄인다.
	const int nDiameter = std::max(1, tOption.nBilateralDiameter);
	cv::bilateralFilter(
		matSrc,
		matDst,
		nDiameter,
		std::max(1.0, tOption.dBilateralSigmaColor),
		std::max(1.0, tOption.dBilateralSigmaSpace));

	return !matDst.empty();
}

bool CImgProc::ApplyClaheAndGamma(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const
{
	if (matSrc.empty() || matSrc.channels() != 3) return false;

	// 색상에 미치는 영향을 줄이기 위해 Lab로 변환하고 밝기(L) 채널만 보정한다.
	cv::Mat matLab;
	cv::cvtColor(matSrc, matLab, cv::COLOR_BGR2Lab);

	std::vector<cv::Mat> vecChannels;
	cv::split(matLab, vecChannels);

	const int nTileSize = std::max(2, tOption.nClaheTileSize);
	cv::Ptr<cv::CLAHE> pClahe = cv::createCLAHE(std::max(0.1, tOption.dClaheClipLimit), cv::Size(nTileSize, nTileSize));
	pClahe->apply(vecChannels[0], vecChannels[0]);

	cv::merge(vecChannels, matLab);
	cv::Mat matClahe;
	cv::cvtColor(matLab, matClahe, cv::COLOR_Lab2BGR);

	// 명시적인 감마값이 없으면 현재 영상의 평균 밝기로부터 자동 결정한다.
	double dGamma = tOption.dGamma;
	if (dGamma <= 0.0) dGamma = CalculateAutoGamma(matClahe);

	dGamma = std::clamp(dGamma, 0.35, 3.0);

	// 모든 픽셀에 pow를 반복하지 않도록 0~255 변환표를 한 번 만들어 적용한다.
	cv::Mat matLut(1, 256, CV_8U);
	for (int i = 0; i < 256; ++i)
	{
		matLut.at<uchar>(i) = cv::saturate_cast<uchar>(std::pow(i / 255.0, dGamma) * 255.0);
	}

	cv::LUT(matClahe, matLut, matDst);
	return !matDst.empty();
}

bool CImgProc::DetectDocumentContour(
	const cv::Mat& matSrc,
	cv::Mat& matCanny,
	std::vector<cv::Point2f>& vecCorners,
	const TImgProcOption& tOption) const
{
	// 실패 시에도 호출자가 이전 검출 결과를 오인하지 않도록 출력부터 비운다.
	vecCorners.clear();
	matCanny.release();

	if (matSrc.empty()) return false;

	cv::Mat matGray;
	if (matSrc.channels() == 3) cv::cvtColor(matSrc, matGray, cv::COLOR_BGR2GRAY);
	else matGray = matSrc;

	// 미세 노이즈를 줄인 뒤 Canny로 밝기 변화가 큰 문서/글자 경계를 추출한다.
	cv::GaussianBlur(matGray, matGray, cv::Size(5, 5), 0.0);
	cv::Canny(
		matGray,
		matCanny,
		std::max(0.0, tOption.dCannyThreshold1),
		std::max(tOption.dCannyThreshold1 + 1.0, tOption.dCannyThreshold2));

	// 끊어진 에지를 닫힘 연산으로 이어 문서 외곽선이 하나의 윤곽이 되도록 한다.
	const cv::Mat matKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::morphologyEx(matCanny, matCanny, cv::MORPH_CLOSE, matKernel, cv::Point(-1, -1), 2);

	std::vector<std::vector<cv::Point>> vecContours;
	cv::findContours(matCanny.clone(), vecContours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	// 너무 작은 사각형(글자, 아이콘 등)을 문서로 오검출하지 않도록 면적 하한을 둔다.
	const double dImageArea = static_cast<double>(matSrc.cols) * matSrc.rows;
	const double dMinArea = dImageArea * std::clamp(tOption.dMinDocumentAreaRatio, 0.01, 0.95);

	// 조건을 만족하는 볼록 사각형 중 가장 넓은 것을 문서 외곽으로 선택한다.
	double dBestArea = 0.0;
	std::vector<cv::Point> vecBestCorners;

	for (const auto& vecContour : vecContours)
	{
		const double dArea = std::fabs(cv::contourArea(vecContour));
		if (dArea < dMinArea || dArea <= dBestArea) continue;

		// 윤곽선을 둘레의 2% 허용 오차로 단순화해 꼭지점이 네 개인지 검사한다.
		const double dPerimeter = cv::arcLength(vecContour, true);
		std::vector<cv::Point> vecApprox;
		cv::approxPolyDP(vecContour, vecApprox, dPerimeter * 0.02, true);

		if (vecApprox.size() == 4 && cv::isContourConvex(vecApprox))
		{
			dBestArea = dArea;
			vecBestCorners = std::move(vecApprox);
		}
	}

	if (vecBestCorners.size() != 4) return false;

	vecCorners = OrderCorners(vecBestCorners);

	return vecCorners.size() == 4;
}

bool CImgProc::CorrectPerspective(const cv::Mat& matSrc, const std::vector<cv::Point2f>& vecCorners, cv::Mat& matDst) const
{
	if (matSrc.empty() || vecCorners.size() != 4) return false;

	// 서로 마주 보는 두 변 중 긴 길이를 출력 사각형의 너비/높이로 사용한다.
	const double dTopWidth = cv::norm(vecCorners[1] - vecCorners[0]);
	const double dBottomWidth = cv::norm(vecCorners[2] - vecCorners[3]);
	const double dLeftHeight = cv::norm(vecCorners[3] - vecCorners[0]);
	const double dRightHeight = cv::norm(vecCorners[2] - vecCorners[1]);

	const int nWidth = static_cast<int>(std::round(std::max(dTopWidth, dBottomWidth)));
	const int nHeight = static_cast<int>(std::round(std::max(dLeftHeight, dRightHeight)));
	if (nWidth < 2 || nHeight < 2) return false;

	// 검출된 네 점이 대응할 정면 직사각형의 목표 좌표.
	const std::vector<cv::Point2f> vecDestination =
	{
		cv::Point2f(0.0f, 0.0f),
		cv::Point2f(static_cast<float>(nWidth - 1), 0.0f),
		cv::Point2f(static_cast<float>(nWidth - 1), static_cast<float>(nHeight - 1)),
		cv::Point2f(0.0f, static_cast<float>(nHeight - 1))
	};

	// 원본 사각형에서 목표 사각형으로 가는 3x3 원근 변환 행렬을 계산해 워핑한다.
	const cv::Mat matTransform = cv::getPerspectiveTransform(vecCorners, vecDestination);
	cv::warpPerspective(
		matSrc,
		matDst,
		matTransform,
		cv::Size(nWidth, nHeight),
		cv::INTER_CUBIC,
		cv::BORDER_REPLICATE);

	return !matDst.empty();
}

bool CImgProc::RemoveShadow(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const
{
	if (matSrc.empty()) return false;

	// 컬러 영상은 채널별로 같은 정규화를 수행한 뒤 다시 합친다.
	std::vector<cv::Mat> vecChannels;
	if (matSrc.channels() == 1) vecChannels.push_back(matSrc);
	else cv::split(matSrc, vecChannels);

	const int nKernelSize = MakeOdd(tOption.nShadowKernelSize, 3);
	const cv::Mat matKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(nKernelSize, nKernelSize));

	for (cv::Mat& matChannel : vecChannels)
	{
		// 큰 커널의 닫힘 연산으로 글자보다 완만하게 변하는 배경 조명을 추정한다.
		cv::Mat matBackground;
		cv::morphologyEx(matChannel, matBackground, cv::MORPH_CLOSE, matKernel);

		cv::Mat matFloatChannel;
		cv::Mat matFloatBackground;
		matChannel.convertTo(matFloatChannel, CV_32F);
		matBackground.convertTo(matFloatBackground, CV_32F);
		matFloatBackground += 1.0f;

		// 픽셀을 배경 밝기로 나누어 조명 변화/그림자를 평탄화하고 0~255로 복원한다.
		cv::divide(matFloatChannel, matFloatBackground, matFloatChannel, 255.0);
		matFloatChannel.convertTo(matChannel, CV_8U);
	}

	if (vecChannels.size() == 1) matDst = vecChannels[0];
	else cv::merge(vecChannels, matDst);

	return !matDst.empty();
}

bool CImgProc::BinarizeAndSharpen(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const
{
	if (matSrc.empty()) return false;

	cv::Mat matGray;

	if (matSrc.channels() == 3) cv::cvtColor(matSrc, matGray, cv::COLOR_BGR2GRAY);
	else matGray = matSrc.clone();

	// 원본에서 저주파(흐린) 성분을 빼는 언샤프 마스크 방식으로 글자 경계를 강조한다.
	cv::Mat matBlurred;
	cv::GaussianBlur(matGray, matBlurred, cv::Size(0, 0), 1.2);

	cv::Mat matSharpened;
	const double dAmount = std::clamp(tOption.dSharpenAmount, 0.0, 3.0);
	cv::addWeighted(matGray, 1.0 + dAmount, matBlurred, -dAmount, 0.0, matSharpened);

	// 위치별 주변 밝기를 기준으로 임계값을 정해 조명이 균일하지 않은 문서도 이진화한다.
	const int nBlockSize = MakeOdd(tOption.nAdaptiveBlockSize, 3);
	cv::adaptiveThreshold(
		matSharpened,
		matDst,
		255,
		cv::ADAPTIVE_THRESH_GAUSSIAN_C,
		cv::THRESH_BINARY,
		nBlockSize,
		tOption.dAdaptiveC);

	return !matDst.empty();
}

bool CImgProc::LoadImageFile(const CString& csImageFileName, cv::Mat& matImage) const
{
	matImage.release();
	if (csImageFileName.IsEmpty()) return false;

	CFile file;
	if (!file.Open(csImageFileName, CFile::modeRead | CFile::shareDenyNone)) return false;

	const ULONGLONG nFileSize = file.GetLength();
	if (nFileSize == 0 || nFileSize > static_cast<ULONGLONG>(std::numeric_limits<size_t>::max())) return false;

	// 경로는 MFC가 처리하고, OpenCV에는 메모리의 파일 바이트를 넘겨 디코딩한다.
	std::vector<uchar> vecData(static_cast<size_t>(nFileSize));
	size_t nOffset = 0;
	// CFile::Read의 UINT 크기 제한을 고려해 최대 1 MiB씩 나누어 읽는다.
	while (nOffset < vecData.size())
	{
		const size_t nRemain = vecData.size() - nOffset;
		const UINT nReadSize = static_cast<UINT>(std::min<size_t>(nRemain, 1024 * 1024));
		const UINT nRead = file.Read(vecData.data() + nOffset, nReadSize);
		
		if (nRead == 0) return false;

		nOffset += nRead;
	}

	matImage = cv::imdecode(vecData, cv::IMREAD_UNCHANGED);
	
	return !matImage.empty();
}

double CImgProc::CalculateAutoGamma(const cv::Mat& matSrc) const
{
	cv::Mat matGray;
	if (matSrc.channels() == 3) cv::cvtColor(matSrc, matGray, cv::COLOR_BGR2GRAY);
	else matGray = matSrc;

	// mean^gamma = 0.5가 되는 gamma를 구한다. 극단값에서 log(0)을 피하도록 평균을 제한한다.
	const double dMean = std::clamp(cv::mean(matGray)[0] / 255.0, 0.01, 0.99);
	return std::log(0.5) / std::log(dMean);
}

std::vector<cv::Point2f> CImgProc::OrderCorners(const std::vector<cv::Point>& vecCorners) const
{
	if (vecCorners.size() != 4) return {};

	// 좌표 합(x+y)은 좌상/우하를, 차(y-x)는 우상/좌하를 구분하는 데 사용한다.
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

int CImgProc::MakeOdd(int nValue, int nMinimum) const
{
	// adaptiveThreshold와 모폴로지 커널은 중심 픽셀이 있는 홀수 크기를 요구한다.
	int nResult = std::max(nValue, nMinimum);
	if ((nResult % 2) == 0) ++nResult;
	return nResult;
}
