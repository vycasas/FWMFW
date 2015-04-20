#if !defined(DOTSLASHZERO_FWMFW_UTILS_HXX)
#define DOTSLASHZERO_FWMFW_UTILS_HXX

#include <functional>
#include <string>
#include <vector>

namespace FWMFW
{
    // utilities for useful operations.
    namespace Utils
    {
        // trims front and rear characters using the provided predicate
        std::string trimChars(const std::string& str, std::function<bool(int)>&& trimPred);

        std::string trimWhiteSpaces(const std::string& str);

        std::string replaceChars(
            const std::string& str, const char findChar, const char replacement, bool stopAtFirstHit = true
        );

        bool stringStartsWith(const std::string& str, const std::string& beginning);

        bool stringEndsWith(const std::string& str, const std::string& ending);

        bool stringContains(const std::string& str, const std::string& strToFind);

        // converts Windows' wide string (usually in UTF-16) to UTF-8 encoding
        std::string w32WStrToUTF8Str(const std::wstring& wstr);

        std::wstring utf8StrToW32WStr(const std::string& str);

        bool doesDirectoryExist(const std::string& dir);

        bool doesFileExist(const std::string& file);

        std::vector<std::string> getFilesInDirectory(
            const std::string& dir, const std::string& fileEnding, bool traverseAll = false
        );
    } // namespace Utils
} // namespace FWMFW

#endif // !defined(DOTSLASHZERO_FWMFW_UTILS_HXX)
