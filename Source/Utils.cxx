#include "Utils.hxx"

#include <Windows.h>

#include <cctype>

namespace FWMFW
{
	namespace Utils
	{
		std::string trimChars(const std::string& str, std::function<bool(int)>&& trimPred)
		{
			if (str.length() == 0) return (str);
			std::string trimmed = str;
			while (trimPred(trimmed[0])) trimmed = trimmed.substr(1);
			while (trimPred(trimmed[trimmed.length() - 1])) trimmed = trimmed.substr(0, trimmed.length() - 1);
			return (trimmed);
		}

		std::string trimWhiteSpaces(const std::string& str)
		{
			return (Utils::trimChars(str, [](int c) -> bool { return (std::isspace(c) != 0); }));
		}

		std::string replaceChars(const std::string& str, const char findChar, const char replacement, bool stopAtFirstHit)
		{
			if (str.length() == 0) return (str);
			std::string result = str;
			for (std::string::size_type idx = 0; idx < result.length(); idx++) {
				if (result[idx] == findChar) {
					result[idx] = replacement;
					if (stopAtFirstHit) break;
				}
			}
			return (result);
		}

		bool stringStartsWith(const std::string& str, const std::string& beginning)
		{
			if (str.length() < beginning.length()) return (false);
			return (str.substr(0, beginning.length()).compare(beginning) == 0);
		}

		bool stringEndsWith(const std::string& str, const std::string& ending)
		{
			if (str.length() < ending.length()) return (false);
			return (str.substr(str.length() - ending.length()).compare(ending) == 0);
		}

		bool stringContains(const std::string& str, const std::string& strToFind)
		{
			if (str.length() < strToFind.length()) return (false);
			return (str.find(strToFind) != std::string::npos);
		}

		std::string w32WStrToUTF8Str(const std::wstring& wstr)
		{
			// let's use the Windows API function: WideCharToMultiByte

			// 1st, count the number of bytes needed to store the converted UTF-8 string
			int bytesRequired = WideCharToMultiByte(
				CP_UTF8,
				WC_ERR_INVALID_CHARS,
				wstr.c_str(),
				static_cast<int>(wstr.length()),
				NULL,
				0,
				NULL,
				NULL
			);

			// then do the actual conversion
			std::string result(static_cast<std::string::size_type>(bytesRequired), '\0');
			WideCharToMultiByte(
				CP_UTF8,
				WC_ERR_INVALID_CHARS,
				wstr.c_str(),
				static_cast<int>(wstr.length()),
				&(result.front()),
				static_cast<int>(result.length()),
				NULL,
				NULL
			);

			return (result);
		}

		std::wstring utf8StrToW32WStr(const std::string& str)
		{
			// same as the opposite function
			int charsRequired = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), static_cast<int>(str.length()), NULL, 0);

			std::wstring result(static_cast<std::wstring::size_type>(charsRequired), L'\0');
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), static_cast<int>(str.length()), &(result.front()), static_cast<int>(result.length()));

			return (result);
		}

		bool doesDirectoryExist(const std::string& dir)
		{
			auto attributes = GetFileAttributesA(dir.c_str());
			return ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY));
		}

		bool doesFileExist(const std::string& file)
		{
			auto attributes = GetFileAttributesA(file.c_str());
			return (attributes != INVALID_FILE_ATTRIBUTES);
		}

		// RVO (and/or copy elision & move operations) should optimize the return operation (i.e. no copying will happen).
		std::vector<std::string> getFilesInDirectory(const std::string& dir, const std::string& fileEnding, bool traverseAll)
		{
			std::vector<std::string> result;
			WIN32_FIND_DATAA findData;
			HANDLE hFind = INVALID_HANDLE_VALUE;

			// must end with "*"
			std::string dirF = dir + "*";

			hFind = FindFirstFileA(dirF.c_str(), &findData);
			if (hFind == INVALID_HANDLE_VALUE) {
				return (result);
			}

			do {
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && traverseAll) {
					// avoid the results '.' and '..'
					std::string subdirname{ findData.cFileName };
					if (subdirname.compare(".") != 0 && subdirname.compare("..") != 0) {
						auto subdir = dir + std::string(findData.cFileName) + "\\";
						auto subdirFiles = Utils::getFilesInDirectory(subdir, fileEnding, traverseAll);
						result.insert(result.cend(), subdirFiles.cbegin(), subdirFiles.cend());
					}
				}
				else {
					std::string file(findData.cFileName);
					if (Utils::stringEndsWith(file, fileEnding)) {
						result.push_back(dir + file);
					}
				}
			} while (FindNextFileA(hFind, &findData) != 0);

			FindClose(hFind);

			return (result);
		}
	} // namespace Utils
} // namespace FWMFW
