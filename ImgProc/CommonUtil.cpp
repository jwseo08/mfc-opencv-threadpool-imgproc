#include "pch.h"
#include "CommonUtil.h"

#include <iostream>
#include <vector>
#include <cctype>
#include <fstream>
#include <stdio.h>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <shobjidl.h>

namespace fs = std::filesystem;


std::string ToLowerString(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char a)
		{
			return (char)(std::tolower(a));
		});

	return str;
}

bool IsImgFile(const std::string& fileName)
{
	std::string ext = fs::path(fileName).extension().string();
	ext = ToLowerString(ext);

	if (ext == ".bmp" ||
		ext == ".jpg" ||
		ext == ".jpeg" ||
		ext == ".gif" ||
		ext == ".tif" ||
		ext == ".png")
		//ext == ".ppm" )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// 실행경로 찾기
std::string GetExePath()
{
	TCHAR buffer[MAX_PATH];
	if (GetModuleFileName(NULL, buffer, MAX_PATH) == 0)
	{
		// 실패 시 빈 문자열 반환
		return std::string();
	}

	// generic 형식으로 경로를 가져옴
	std::string exePath = fs::path(buffer).parent_path().generic_string();

	return exePath;
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


