#pragma once

#define DCOBJTYPE_FNT	1
#define DCOBJTYPE_PEN	2
#define DCOBJTYPE_BRUSH	3

#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "opencv2/opencv.hpp"
#include "add_gdiplus.h"


int Mat2CImage(cv::Mat* mat, CImage& img);
CImage* ImgBufToCImg(unsigned char* pBuf, UINT nBufSize);
Bitmap* ImgBufToBitmap(unsigned char* pBuf, UINT nBufSize);

int OpenFileDlgMultiFile(CWnd* pParent, CStringArray& csarPathFileName);
int OpenFileDlgSingleFile(CWnd* pParent, CString& csPathFileName);
int SaveFileDlg(CWnd* pParent, CString& csSavePathFileName);

CString GetFileNameFromPathFile(CString csPathFile);
CString GetFileExtFromPathFile(CString csPathFile);

bool IsImgFile(CString csFileName);
CString GetModulePath();

CString GetTimeStamp();

CString ReadIniFile(CString csSection, CString csKey, CString csDefaultVal);

void WriteLog(const CString& logMessage, const CString& logFilePath);
CString CreateLogDir(CString csRunFunc);

int GetLogUseFromIni();

int GetImgPathFileFromFolder(CString csPath, CStringArray& csarPathFile);
void ShowFolderPickerDlg(CString& csPath);
bool CopyFileSafe(const CString& srcFile, const CString& destFile);
void OpenFolderWithShell(const CString& folderPath);
void RemoveNonNumericAndDot(CString& str);

bool IsFolderExist(CString csFolderPath);
bool IsFileExist(CString csPathFile);

std::string MakeAbsPath(const std::string& relativePath, bool exist = false);
bool SelectFolder(HWND hParent, CString& outFolder);
std::string GetCurrentDateTime(bool formated = false);
std::string MakeDirByDateTime(const std::string& parentPath, const std::string& dirNamePrefix);
int SaveLog(const std::string& pathFile, const std::ostringstream& oss);
std::vector<std::string> ListFileWithExt(const std::string& folderPath, const std::vector<std::string>& extensionList);

int ListFileByExt(const std::string& folderPath, 
	const std::vector<std::string>& extensionList,
	std::vector<std::string>& vFileList, 
	const std::atomic<bool>& stopRequest);

CString GetExePath();
