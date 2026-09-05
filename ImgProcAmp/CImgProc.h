#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

// 이미지 처리 파이프라인 설정값
struct TImgProcOption
{
	int nMaxWidth = 1600;               // 처리할 영상의 최대 가로 길이
	int nMaxHeight = 1600;              // 처리할 영상의 최대 세로 길이

	int nBilateralDiameter = 7;         // 양방향 필터가 참조하는 픽셀 이웃의 지름
	double dBilateralSigmaColor = 50.0; // 색 차이 허용 범위 - 클수록 다른 색상도 평활화
	double dBilateralSigmaSpace = 50.0; // 공간 거리 허용 범위 - 클수록 먼 픽셀까지 영향

	double dClaheClipLimit = 2.0;       // clahe 대비 증폭 제한값 - 과도한 노이즈 증폭 억제
	int nClaheTileSize = 8;             // clahe의 가로세로 타일 개수 - grid 크기
	double dGamma = 0.0;                // 감마값 - 0 이하면 영상의 평균 밝기로 자동 계산

	double dCannyThreshold1 = 50.0;     // canny 에지 검출 최소 임계값
	double dCannyThreshold2 = 150.0;    // canny 에지 검출 최대 임계값
	double dMinDocumentAreaRatio = 0.15;// 문서 영역이 전체 영상에서 차지하는 최소 면적 비율

	int nShadowKernelSize = 21;         // 배경 조명 추정 모폴로지 커널 크기 - 홀수로 보정.
	int nAdaptiveBlockSize = 31;        // 적응형 이진화 국소 영역 크기 - 홀수로 보정
	double dAdaptiveC = 7.0;            // 국소 평균에서 빼는 상수 - 클수록 흰색으로 판정되는 범위 넓어짐
	double dSharpenAmount = 1.0;        // 언샤프 마스크 강도 - 내부에서 0.0~3.0으로 제한
};

// process 또는 process and save가 호출자에게 돌려주는 처리 결과, 중간 진단 정보.
// cimgproc 자체는 작업 상태를 보관하지 않으므로 호출마다 이 구조체가 초기화된다.
struct TImgProcResult
{
	bool bSuccess = false;              // 전체 파이프라인 정상 완료 여부
	bool bPerspectiveApplied = false;   // 문서 사각형을 찾고 원근 보정이 적용되었는지 여부
	CString csErrorMessage;             // 작업이 실패한 경우 실패 내용 - 성공한 경우 empty

	cv::Mat matResult;                  // 최종 선명화 이진화된 8비트 흑백 영상
	cv::Mat matCanny;                   // 문서 윤곽 검출에 사용한 Canny 에지 영상 - 검증용.
	std::vector<cv::Point2f> vecDocumentCorners; // 문서 꼭지점 - 좌상, 우상, 우하, 좌하 순서
};

// 촬영한 문서 이미지를 스캔 영상처럼 정리하는 무상태(stateless) 처리 클래스.
// 처리 순서:
// 파일 로드 -> 크기/채널 정규화 -> 노이즈 제거 -> 대비/감마 보정
// -> 문서 검출/원근 보정(선택) -> 그림자 완화 -> 선명화/이진화
class CImgProc
{
public:
	CImgProc() = default;
	~CImgProc() = default;

	// 입력 파일을 읽어 전체 처리 파이프라인을 실행하고 결과를 tResult에 반환한다.
	// 성공 시 true와 matResult를, 실패 시 false와 csErrorMessage를 반환한다.
	bool Process(const CString& csImageFileName, TImgProcResult& tResult, const TImgProcOption& tOption = TImgProcOption()) const;

	// Process와 동일한 파이프라인을 실행한 뒤 최종 영상을 csSaveFileName에 저장한다.
	// 영상 처리뿐 아니라 파일 저장까지 성공해야 true를 반환한다.
	bool ProcessAndSave(const CString& csImageFileName, const CString& csSaveFileName, TImgProcResult& tResult, const TImgProcOption& tOption) const;

	// 최대 크기에 맞춰 축소하고 1/3/4채널 입력을 3채널 BGR로 정규화한다.
	// 작은 원본은 확대하지 않으며 종횡비를 유지한다.
	bool ResizeAndConvertColor(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// 양방향 필터로 글자/문서 경계는 보존하면서 촬영 노이즈를 완화한다.
	bool RemoveNoise(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// Lab 색 공간의 밝기(L) 채널에 CLAHE를 적용한 뒤 감마 보정한다.
	// 색상 성분을 가능한 한 유지하면서 조명 차이와 낮은 대비를 개선한다.
	bool ApplyClaheAndGamma(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// Canny 에지와 윤곽선 분석으로 가장 큰 볼록 사각형 문서 후보를 찾는다.
	// matCanny에는 에지 영상을, vecCorners에는 정렬된 네 꼭지점을 반환한다.
	bool DetectDocumentContour(
		const cv::Mat& matSrc,
		cv::Mat& matCanny,
		std::vector<cv::Point2f>& vecCorners,
		const TImgProcOption& tOption) const;

	// 좌상/우상/우하/좌하 순서의 네 꼭지점을 직사각형에 사상해 원근을 보정한다.
	bool CorrectPerspective(const cv::Mat& matSrc, const std::vector<cv::Point2f>& vecCorners, cv::Mat& matDst) const;

	// 채널별 모폴로지 닫힘 연산으로 배경 조명을 추정하고 원본을 배경으로 정규화한다.
	bool RemoveShadow(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

	// 언샤프 마스크로 글자 경계를 강조하고 적응형 임계처리로 최종 흑백 영상을 만든다.
	bool BinarizeAndSharpen(const cv::Mat& matSrc, cv::Mat& matDst, const TImgProcOption& tOption) const;

private:
	// MFC CFile로 전체 바이트를 읽고 imdecode로 디코딩한다.
	// 한글 등 비 ASCII 경로를 std::string으로 직접 넘길 때 생기는 경로 문제를 피한다.
	bool LoadImageFile(const CString& csImageFileName, cv::Mat& matImage) const;
	
	// 영상 평균 밝기가 중간 밝기(0.5)에 가까워지도록 감마값을 계산한다.
	double CalculateAutoGamma(const cv::Mat& matSrc) const;
	
	// 임의 순서의 네 꼭지점을 좌상, 우상, 우하, 좌하 순서로 정렬한다.
	std::vector<cv::Point2f> OrderCorners(const std::vector<cv::Point>& vecCorners) const;
	
	// OpenCV 커널/블록 크기 조건을 만족하도록 최솟값 이상의 홀수로 보정한다.
	int MakeOdd(int nValue, int nMinimum) const;
};
