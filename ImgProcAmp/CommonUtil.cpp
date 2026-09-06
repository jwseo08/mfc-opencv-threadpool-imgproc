#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "pch.h"
#include "CommonUtil.h"

#include <iostream>
#include <string>
#include <cctype>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include <shobjidl.h>

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>




// opencv 의 mat 를 CImage 객체로 변경 
int Mat2CImage(cv::Mat* mat, CImage& img)
{
	if (!mat || mat->empty())
		return -1;
	if (!mat->data) {
		return -1;
	}
	int nBPP = mat->channels() * 8;
	img.Create(mat->cols, mat->rows, nBPP);
	if (nBPP == 8)
	{
		static RGBQUAD pRGB[256];
		for (int i = 0; i < 256; i++)
			pRGB[i].rgbBlue = pRGB[i].rgbGreen = pRGB[i].rgbRed = i;
		img.SetColorTable(0, 256, pRGB);
	}
	uchar* psrc = mat->data;
	uchar* pdst = (uchar*)img.GetBits();
	int imgPitch = img.GetPitch();
	for (int y = 0; y < mat->rows; y++)
	{
		memcpy(pdst, psrc, mat->cols * mat->channels());//mat->step is incorrect for those images created by roi (sub-images!)
		psrc += mat->step;
		pdst += imgPitch;
	}

	return 0;
}


CImage* ImgBufToCImg(unsigned char* pBuf, UINT nBufSize)
{
	IStream* pStream = SHCreateMemStream((BYTE*)pBuf, nBufSize);
	if (pStream != NULL)
	{
		CImage* pImg = new CImage;
		pImg->Load(pStream);
		pStream->Release();

		return pImg;
	}
	else
	{
		return NULL;
	}
}

Bitmap* ImgBufToBitmap(unsigned char* pBuf, UINT nBufSize)
{
	IStream* pStream = SHCreateMemStream((BYTE*)pBuf, nBufSize);
	if (pStream != NULL)
	{
		Bitmap* pBitImg = new Bitmap(pStream);
		pStream->Release();

		return pBitImg;
	}
	else
	{
		return NULL;
	}
}

int OpenFileDlgMultiFile(CWnd* pParent, CStringArray& csarPathFileName)
{
	int nRet = IDCANCEL;

	//char szFilter[] = "Image(*.jpg;*.jpeg;*.bmp;*.png)|*.jpg;*.jpeg;*.bmp;*.png|JPEG(*.jpg;*.jpeg)|*.jpg;*.jpeg|BITMAP(*.bmp)|*.bmp|PNG(*.png)|*.png|All Files(*.*)|*.*||";
	//CString csFilter = _T("JPEG(*.jpg)|*.jpg|");	// 선택할 파일 종류

	CString csFilter = _T("Image(*.jpg;*.jpeg;*.bmp;*.png)|*.jpg;*.jpeg;*.bmp;*.png|JPEG(*.jpg;*.jpeg)|*.jpg;*.jpeg|BITMAP(*.bmp)|*.bmp|PNG(*.png)|*.png|All Files(*.*)|*.*||");
	CFileDialog FileDlg(TRUE, NULL, NULL, OFN_ALLOWMULTISELECT, csFilter, pParent);

	// 선택할 파일 최대 숫자 - 메모리 부족 방지
	const int nMaxFile = 400;
	const int nBuffSize = (nMaxFile * (MAX_PATH + 1)) + 1;
	CString csFileList = "";

	FileDlg.GetOFN().lpstrFile = csFileList.GetBuffer(nBuffSize);
	FileDlg.GetOFN().nMaxFile = nBuffSize;

	if (FileDlg.DoModal() == IDOK)
	{
		CString csPathFileName = "";

		for (POSITION pos = FileDlg.GetStartPosition(); pos != NULL;)
		{
			// 전체삭제는 ResetContent
			csPathFileName = FileDlg.GetNextPathName(pos);
			csarPathFileName.Add(csPathFileName);
		}

		nRet = IDOK;
	}

	csFileList.ReleaseBuffer();

	return nRet;
}

int OpenFileDlgSingleFile(CWnd* pParent, CString& csPathFileName)
{
	int nRet = IDCANCEL;

	CString csFilter = _T("Image(*.jpg;*.jpeg;*.bmp;*.png)|*.jpg;*.jpeg;*.bmp;*.png|JPEG(*.jpg;*.jpeg)|*.jpg;*.jpeg|BITMAP(*.bmp)|*.bmp|PNG(*.png)|*.png|All Files(*.*)|*.*||");
	//CString csFilter = _T("Image(*.jpg;*.jpeg;*.bmp;*.png;*.ppm)|*.jpg;*.jpeg;*.bmp;*.png;*.ppm|JPEG(*.jpg;*.jpeg)|*.jpg;*.jpeg|BITMAP(*.bmp)|*.bmp|PNG(*.png)|*.png|PPM(*.ppm)|*.ppm|All Files(*.*)|*.*||");
	CFileDialog dlg(TRUE, NULL, NULL, 0/*OFN_HIDEREADONLY*/, csFilter);

	if (IDOK == dlg.DoModal())
	{
		csPathFileName = dlg.GetPathName();

		nRet = IDOK;
	}

	return nRet;
}

int SaveFileDlg(CWnd* pParent, CString& csSavePathFileName)
{
	int nRet = IDCANCEL;

	CString csFilter = _T("JPEG Files (*.jpg;*.jpeg)|*.jpg;*.jpeg|PNG Files (*.png)|*.png|BMP Files (*.bmp)|*.bmp|All Files (*.*)|*.*||");
	CFileDialog dlg(FALSE, _T("jpg"), _T("image"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFilter);

	if (dlg.DoModal() == IDOK)
	{
		CString csPathFile = dlg.GetPathName();
		CString csExtension = dlg.GetFileExt().MakeLower();

		if (csExtension.IsEmpty())
		{
			switch (dlg.m_ofn.nFilterIndex)
			{
			case 1:
				csPathFile += _T(".jpg");
				break;  // JPEG
			case 2:
				csPathFile += _T(".png");
				break;  // PNG
			case 3:
				csPathFile += _T(".bmp");
				break;  // BMP
			default:
				csPathFile += _T(".jpg");
				break;  // 기본 JPEG
			}
		}

		csSavePathFileName = csPathFile;
		nRet = IDOK;
	}

	return nRet;
}

// 전체 경로에서 경로를 제외한 파일이름 - 확장자 포함
CString GetFileNameFromPathFile(CString csPathFile)
{
	CString csFileName = fs::path(csPathFile.GetString()).filename().string().c_str();

	return csFileName;
}


// 전체 경로나 파일 이름에서 확장자 가저옴
CString GetFileExtFromPathFile(CString csPathFile)
{
	CString csExt = csPathFile;

	int nIndex = csExt.ReverseFind('.');
	csExt = csExt.Mid(nIndex + 1, csExt.GetLength() - nIndex);

	return csExt;
}


bool IsImgFile(CString csFileName)
{
	int nIndex = csFileName.ReverseFind('.');
	CString csExt = csFileName.Mid(nIndex + 1, csFileName.GetLength() - nIndex);

	if ((csExt.CompareNoCase(TEXT("bmp")) == 0) ||
		(csExt.CompareNoCase(TEXT("jpg")) == 0) ||
		(csExt.CompareNoCase(TEXT("jpeg")) == 0) ||
		(csExt.CompareNoCase(TEXT("gif")) == 0) ||
		(csExt.CompareNoCase(TEXT("tif")) == 0) ||
		(csExt.CompareNoCase(TEXT("png")) == 0))
		//(csExt.CompareNoCase(TEXT("ppm")) == 0) )
	{
		return true;
	}
	else
	{
		return false;
	}
}


CString GetModulePath()
{
	CString csModulePath = "";

	char strPath[MAX_PATH] = "";
	GetModuleFileName(NULL, strPath, MAX_PATH);

	csModulePath = strPath;
	int nIndex = csModulePath.ReverseFind('\\');
	csModulePath.Delete(nIndex, csModulePath.GetLength() - nIndex);

	return csModulePath;
}


CString GetTimeStamp()
{
	// 현재 시간 가져오기
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];

	localtime_s(&tstruct, &now);
	//tstruct = *localtime(&now);

	// 시간 정보를 "년월일시분초" 형식으로 변환
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tstruct);

	CString csTimeStamp = "";
	csTimeStamp.Format("%s", buf);

	return csTimeStamp;
}


CString ReadIniFile(CString csSection, CString csKey, CString csDefaultVal)
{
	CString iniPath = GetModulePath() + "\\setup.ini"; // INI 파일 경로
	CString section = csSection;                     // 섹션 이름
	CString key = csKey;                         // 키 이름
	CString defaultValue = csDefaultVal;             // 기본값
	CString value;                                        // 값을 저장할 변수
	TCHAR buffer[256];                                    // 값을 읽어올 버퍼
	DWORD bufferSize = 256;                               // 버퍼 크기

	// INI 파일에서 값을 읽어오기
	GetPrivateProfileString(
		section,       // 섹션 이름
		key,           // 키 이름
		defaultValue,  // 기본값
		buffer,        // 버퍼
		bufferSize,    // 버퍼 크기
		iniPath        // INI 파일 경로
	);

	// CString 변수에 결과 저장
	value = buffer;

	return value;
}

void WriteLog(const CString& logMessage, const CString& logFilePath)
{
	// 파일 열기 (쓰기 모드, 파일이 없으면 생성하고, 있으면 내용 추가)
	CStdioFile logFile;
	CFileException ex;

	if (logFile.Open(logFilePath, CFile::modeWrite | CFile::modeNoTruncate | CFile::modeCreate, &ex))
	{
		// 파일 끝으로 이동하여 내용을 추가할 준비
		logFile.SeekToEnd();

		// 현재 시간을 가져오기
		CTime currentTime = CTime::GetCurrentTime();
		CString timestamp = currentTime.Format(_T("[%Y-%m-%d %H:%M:%S] "));

		// 타임스탬프와 로그 메시지를 파일에 작성
		logFile.WriteString(timestamp + logMessage + _T("\n"));

		// 파일 닫기
		logFile.Close();
	}
	else
	{
		// 파일 열기에 실패했을 경우 오류 메시지를 표시
		TCHAR errorMsg[1024];
		ex.GetErrorMessage(errorMsg, 1024);
		AfxMessageBox(CString(_T("Failed to write log: ")) + errorMsg);
	}
}

CString CreateLogDir(CString csRunFunc)
{
	// 현재 시간을 가져오기
	CTime currentTime = CTime::GetCurrentTime();
	CString timestamp = currentTime.Format(_T("%Y-%m-%d %H-%M-%S-"));

	CString csDirPath = GetModulePath() + "\\" + timestamp + csRunFunc;

	if (PathFileExists(csDirPath) == FALSE)
	{
		CreateDirectory(csDirPath, NULL);
	}

	return csDirPath;
}

int GetLogUseFromIni()
{
	CString csVal = ReadIniFile("setting", "log_use", "1");
	int nLogUse = atoi((LPCTSTR)csVal);

	return nLogUse;
}

int GetImgPathFileFromFolder(CString csPath, CStringArray& csarPathFile)
{
	csPath.Format("%s\\%s", csPath, "*.*");

	CFileFind Finder;
	BOOL bWorking = Finder.FindFile(csPath);

	CString csFileName = "";
	CString csPathFile = "";
	int nFileCnt = 0;
	while (bWorking)
	{
		bWorking = Finder.FindNextFile();

		if (Finder.IsDirectory() || Finder.IsDots()) continue;

		csFileName = Finder.GetFileName();

		if (IsImgFile(csFileName) == true)
		{
			csPathFile = Finder.GetFilePath();
			csarPathFile.Add(csPathFile);

			nFileCnt++;
		}
		else
		{
			continue;
		}
	}

	return nFileCnt;
}

void ShowFolderPickerDlg(CString& csPath)
{
	CFolderPickerDialog folderDlg;
	folderDlg.m_ofn.lpstrTitle = _T("결과 이미지 저장폴더 선택");

	if (folderDlg.DoModal() == IDOK)
	{
		csPath = folderDlg.GetPathName();
	}
	else
	{
		csPath = "";
	}
}

bool CopyFileSafe(const CString& srcFile, const CString& destFile)
{
	if (CopyFile(srcFile, destFile, FALSE))  // FALSE: 기존 파일 덮어쓰기
	{
		return true;
	}
	else
	{
		DWORD dwError = GetLastError();  // 오류 코드 가져오기
		CString errorMsg;
		errorMsg.Format(_T("파일 복사 오류. 오류 코드: %lu"), dwError);
		AfxMessageBox(errorMsg);
		return false;
	}
}


void OpenFolderWithShell(const CString& folderPath)
{
	// ShellExecute를 사용하여 탐색기로 폴더 열기
	HINSTANCE result = ShellExecute(
		nullptr,        // 부모 창 핸들
		_T("open"),     // 작업: 열기
		folderPath,     // 열 폴더 경로
		nullptr,        // 명령줄 인수
		nullptr,        // 작업 디렉토리
		SW_SHOWNORMAL   // 탐색기 창을 정상 크기로 열기
	);

	if ((UINT64)result <= 32)
		//if (result <= (HINSTANCE)32)
	{
		AfxMessageBox(_T("폴더 열기 작업이 실패했습니다."), MB_ICONERROR);
		//AfxMessageBox(_T("Failed to open folder!"), MB_ICONERROR);
	}
}

void RemoveNonNumericAndDot(CString& str)
{
	for (int i = 0; i < str.GetLength(); )
	{
		TCHAR ch = str[i];
		if (!(isdigit(ch) || ch == _T('.')))
		{
			str.Delete(i); // 숫자나 마침표가 아니면 삭제
		}
		else
		{
			++i; // 유효한 문자면 다음 문자로 이동
		}
	}
}

bool IsFolderExist(CString csFolderPath)
{
	DWORD attributes = GetFileAttributes(csFolderPath);
	return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool IsFileExist(CString csPathFile)
{
	DWORD attributes = GetFileAttributes(csPathFile);

	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	if (attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		return false; // 디렉터리인 경우 false 반환
	}

	return true;
}

// 실행경로 찾기
CString GetExePath()
{
	std::string exePath = "";

	TCHAR buffer[MAX_PATH];
	if (GetModuleFileName(NULL, buffer, MAX_PATH) == 0)
	{
		// 실패 시 빈 문자열 반환
		return CString(exePath.c_str());
	}

	// generic 형식으로 경로를 가져옴
	exePath = fs::path(buffer).parent_path().generic_string();

	return CString(exePath.c_str());
}

// 상대 경로를 절대 경로로 변경하고 경로 정리
std::string MakeAbsPath(const std::string& relativePath, bool exist/*=false*/)
{
	if (exist)
	{
		// 경로가 실제로 존재하는지 검사해야하는 경우
		try
		{
			// 파일이 존재하지 않으면 exception 발생 try catch 필요
			fs::path realPath = fs::canonical(relativePath);
			return realPath.generic_string();
		}
		catch (const fs::filesystem_error& e)
		{
			// 파일이 없거나 권한이 없을 때의 예외 처리
			std::cerr << "make canonical path fail=" << e.what() << '\n';

			// 실패 시 빈 문자열 반환
			return "";
		}
	}
	else
	{
		// 경로가 존재히지는 것과 상관없이 절대 경로 변환
		fs::path realPath = fs::absolute(relativePath).lexically_normal();
		return realPath.generic_string();
	}
}

char ToLowerChar(unsigned char c)
{
	return static_cast<char>(std::tolower(c));
}

std::string ToLowerString(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ToLowerChar);
	return str;
}

std::vector<std::string> ListFileWithExt(const std::string& folderPath, const std::vector<std::string>& extensionList)
{
	std::vector<std::string> files;

	if (!fs::exists(folderPath) || !fs::is_directory(folderPath))
	{
		std::cerr << "wrong path\n";
		return files;
	}

	std::vector<std::string> allowExtList;
	for (const auto& extension : extensionList)
	{
		std::string ext = ToLowerString(extension);
		if (!ext.empty() && ext[0] != '.') ext.insert(ext.begin(), '.');
		
		allowExtList.push_back(ext);
	}

	for (const auto& entry : fs::directory_iterator(folderPath))
	{
		if (entry.is_regular_file())
		{
			std::string srcExt = ToLowerString(entry.path().extension().string());
			for (const auto& compareExt : allowExtList)
			{
				if (srcExt == compareExt)
				{
					files.push_back(entry.path().generic_string());
					break;
				}
			}
		}
	}

	std::sort(files.begin(), files.end());
	for (const auto& file : files)
	{
		std::cout << file << std::endl;
	}

	return files;
}

int ListFileByExt(const std::string& folderPath,
	const std::vector<std::string>& extensionList,
	std::vector<std::string>& vFileList,
	const std::atomic<bool>& stopRequest)
{
	std::vector<std::string> files;

	if (!fs::exists(folderPath) || !fs::is_directory(folderPath))
	{
		std::cerr << "wrong path\n";
		if (vFileList.size() > 0) vFileList = std::vector<std::string>();
		return -1;
	}

	std::vector<std::string> allowExtList;
	for (const auto& extension : extensionList)
	{
		std::string ext = ToLowerString(extension);
		if (!ext.empty() && ext[0] != '.') ext.insert(ext.begin(), '.');

		allowExtList.push_back(ext);
	}

	for (const auto& entry : fs::directory_iterator(folderPath))
	{
		if (stopRequest == true) break;

		if (entry.is_regular_file())
		{
			std::string srcExt = ToLowerString(entry.path().extension().string());
			for (const auto& compareExt : allowExtList)
			{
				if (srcExt == compareExt)
				{
					files.push_back(entry.path().generic_string());
					break;
				}
			}
		}
	}

	if (stopRequest == true)
	{
		return -2;
	}
	else
	{
		std::sort(files.begin(), files.end());
		for (const auto& file : files)
		{
			std::cout << file << std::endl;
		}

		vFileList = std::move(files);
		return 0;
	}
}

bool SelectFolder(HWND hParent, CString& outFolder)
{
	IFileOpenDialog* pDialog = nullptr;

	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pDialog));

	if (FAILED(hr))
	{
		outFolder = "";
		return false;
	}

	DWORD options = 0;
	pDialog->GetOptions(&options);
	pDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

	hr = pDialog->Show(hParent);
	if (SUCCEEDED(hr))
	{
		IShellItem* pItem = nullptr;
		hr = pDialog->GetResult(&pItem);

		if (SUCCEEDED(hr))
		{
			PWSTR pszPath = nullptr;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

			if (SUCCEEDED(hr))
			{
				outFolder = pszPath;
				CoTaskMemFree(pszPath);
			}
			else
			{
				outFolder = "";
			}

			pItem->Release();
		}
	}

	pDialog->Release();

	return SUCCEEDED(hr);
}

std::string GetCurrentDateTime(bool formated/* = false*/)
{
	// 현재 시스템 시간
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);

	// local time 구조체 변환
	std::tm local_tm;
#if defined(_WIN32)
	localtime_s(&local_tm, &now_c); 
#else
	localtime_r(&now_c, &local_tm); 
#endif

	std::stringstream ss;

	if (formated) ss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
	else ss << std::put_time(&local_tm, "%Y%m%d%H%M%S");

	return ss.str();
}

std::string MakeDirByDateTime(const std::string& parentPath, const std::string& dirNamePrefix)
{
	std::string dirPath = MakeAbsPath(parentPath) + "/" + dirNamePrefix + "-" + GetCurrentDateTime();
	if (!fs::exists(dirPath)) fs::create_directories(dirPath);

	return dirPath;
}

int SaveLog(const std::string& pathFile, const std::ostringstream& oss)
{
	// 로그 파일 저장
	std::ofstream file(pathFile);

	if (file)
	{
		file << oss.str();
		return 0;
	}
	else
	{
		return -1;
	}

	return 0;
}

int WriteFileFromBuf(const std::string& filename, const void* data, size_t size)
{
	if (!data && size > 0)return -1;

	FILE* fp = fopen(filename.c_str(), "wb");
	if (!fp)
	{
		perror("file open for writing failed");
		return -2;
	}

	if (size > 0)
	{
		size_t written = fwrite(data, 1, size, fp);

		if (written != size)
		{
			std::cerr << "file write failed" << std::endl;
			fclose(fp);
			return -3;
		}
	}

	if (fclose(fp) != 0)
	{
		perror("file close failed");
		return -4;
	}

	return 0;
}

int ReadFileToVecBuf(const std::string& filename, std::vector<unsigned char>& vBuf)
{
	FILE* fp = fopen(filename.c_str(), "rb");

	if (!fp)
	{
		perror("file opening failed");
		return -1;
	}

#ifdef _WIN32

	if (_fseeki64(fp, 0, SEEK_END) != 0)
	{
		perror("file seek failed");
		fclose(fp);
		return -2;
	}

	long long fileSize = _ftelli64(fp);

	if (fileSize < 0)
	{
		perror("file size calculation failed");
		fclose(fp);
		return -3;
	}

	if (_fseeki64(fp, 0, SEEK_SET) != 0)
	{
		perror("file seek failed");
		fclose(fp);
		return -4;
	}

#else

	if (fseeko(fp, 0, SEEK_END) != 0)
	{
		perror("file seek failed");
		fclose(fp);
		return -2;
	}

	off_t fileSize = ftello(fp);

	if (fileSize < 0)
	{
		perror("file size calculation failed");
		fclose(fp);
		return -3;
	}

	if (fseeko(fp, 0, SEEK_SET) != 0)
	{
		perror("file seek failed");
		fclose(fp);
		return -4;
	}

#endif

	if (static_cast<unsigned long long>(fileSize) > SIZE_MAX)
	{
		std::cerr << "File is too large to fit in memory." << std::endl;
		fclose(fp);
		return -5;
	}

	size_t bufSize = static_cast<size_t>(fileSize);

	if (bufSize == 0)
	{
		vBuf.clear();
		fclose(fp);
		return -6;
	}

	if (vBuf.size() != bufSize) vBuf.resize(bufSize);

	size_t readSize = fread(vBuf.data(), 1, bufSize, fp);

	if (readSize != bufSize)
	{
		std::cerr << "file read failed" << std::endl;
		fclose(fp);
		vBuf.clear();
		return -7;
	}

	fclose(fp);

	return 0;
}

void TestFunc()
{
	
}

//CImage* ImgCvToCImg(cv::Mat* pMatImg)
//{
//	return nullptr;
//}

//Bitmap* GetBitmapFromCv(cv::Mat MatImg)
//{
//	cv::Size size = MatImg.size();
//	Bitmap BitImg(size.width, size.height, MatImg.step1(), PixelFormat24bppRGB, MatImg.data);
//	return &BitImg;
//}
