#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

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

// 이미지 처리 결과 - 호출마다 초기화
struct TImgProcResult
{
	bool bSuccess = false;              // 전체 처리 성공 여부
	bool bPerspectiveApplied = false;   // 원근 보정 적용 여부
	CString csErrorMessage;             // 실패 내용 - 성공 시 empty

	cv::Mat matResult;                  // 최종 이진화 이미지 - 8비트 흑백
	cv::Mat matCanny;                   // 문서 검출용 canny 경계선 이미지
	std::vector<cv::Point2f> vecDocumentCorners; // 문서 꼭지점 - 좌상, 우상, 우하, 좌하 순서
};

// 이미지 처리 클래스
// 원근 보정, 그림자 제거, 선명화, 이진화
class CImgProc
{
public:
	CImgProc() = default;
	~CImgProc() = default;

	// 이미지 처리 - 파이프라인 실행
	bool Process(const CString& csImageFileName, TImgProcResult& tResult, const TImgProcOption& tOption = TImgProcOption()) const;

	// 이미지 처리 및 결과 저장 - process 작업에 결과 이미지 파일 저장 추가
	bool ProcessAndSave(const CString& csImageFileName, const CString& csSaveFileName, TImgProcResult& tResult, const TImgProcOption& tOption) const;

	// 이미지 크기 조정, bgr 3채널 변환
	bool ResizeAndConvertColor(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// 양방향 필터 적용 - 경계선 유지, 노이즈 제거
	bool RemoveNoise(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// clahe 부분 대비 보정, 감마 전체 밝기 보정
	bool ApplyClaheAndGamma(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// 문서 경계선 검출
	bool DetectDocumentContour(
		const cv::Mat& matSrc,
		cv::Mat& matCanny,
		std::vector<cv::Point2f>& vecCorners,
		const TImgProcOption& tOption) const;

	// 원근 보정 - 문서 꼭지점을 직사각형으로 변환
	bool CorrectPerspective(const cv::Mat& matSrc, const std::vector<cv::Point2f>& vecCorners, cv::Mat& matDst) const;

	// 배경 밝기로 픽셀값 보정 - 그림자 완화
	bool RemoveShadow(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// unsharp 마스크 선명화, 적응형 이진화
	bool BinarizeAndSharpen(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

private:
	// 이미지 파일 로드
	bool LoadImageFile(const CString& csImageFileName, cv::Mat& matImage) const;
	
	// 평균 밝기 기준 감마값 계산
	double CalculateAutoGamma(const cv::Mat& matSrc) const;
	
	// 꼭지점 정렬 - 좌상, 우상, 우하, 좌하 순서
	std::vector<cv::Point2f> OrderCorners(const std::vector<cv::Point>& vecCorners) const;
	
	// 커널 크기 설정 시 홀수로 보정
	int MakeOdd(int nValue, int nMinimum) const;
};
