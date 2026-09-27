#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H


#include <string>
#include <vector>
#include <atomic>

std::string MakeAbsPath(const std::string& relativePath, bool exist = false);
bool IsImgFile(const std::string& fileName);
std::string GetExePath();

std::string GetCurrentDateTime(bool formated = false);
std::string MakeDirByDateTime(const std::string& parentPath, const std::string& dirNamePrefix);

int WriteFileFromBuf(const std::string& filename, const void* data, size_t size);
int ReadFileToVecBuf(const std::string& filename, std::vector<unsigned char>& vBuf);

std::vector<std::string> ListFileWithExt(const std::string& folderPath, const std::vector<std::string>& extensionList);

int ListFileByExt(const std::string& folderPath, 
	const std::vector<std::string>& extensionList,
	std::vector<std::string>& vFileList, 
	const std::atomic<bool>& stopRequest);


int SaveLog(const std::string& pathFile, const std::ostringstream& oss);

#endif // !COMMON_UTIL_H
