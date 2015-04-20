#include <Windows.h>
#include <netfw.h>

#include <fstream>
#include <iostream>
#include <unordered_map>

#include "Utils.hxx"
#include "WinNetFW.hxx"

namespace
{
	namespace Constants
	{
		static const std::string RULE_NAME_PREFIX{ "FWMFW_" };
		static const std::string RULE_IN_NAME_PREFIX{ RULE_NAME_PREFIX + "IN_" };
		static const std::string RULE_OUT_NAME_PREFIX{ RULE_NAME_PREFIX + "OUT_" };
	} // namespace Constants
};

int main(int argc, const char* const argv[])
{
	// process command line arguments
	if (argc < 2) {
		std::cerr << "Error: missing argument.\n";
		std::cerr << "Argument must be a file containing a list of files/folders to block.\n";
		std::cerr << "Each file/folder must be specified with full absolute paths." << std::endl;
		return (-2);
	}

	FWMFW::WinNetFW::initialize();

	// parse the text file
	const std::string LIST_FILE{ argv[1] };

	std::unordered_map<std::string, std::string> requestedFilesToBlock;

	try {
		std::ifstream inFile{ LIST_FILE };

		for (std::string line; std::getline(inFile, line);) {
			auto item = FWMFW::Utils::trimWhiteSpaces(line);

			// skip empty lines and lines marked as 'comment' (starts with '#' sign)
			if (item.empty() || item[0] == '#') continue;

			// normalize path separators
			auto itemF = FWMFW::Utils::replaceChars(item, '/', '\\', false);

			if (FWMFW::Utils::doesDirectoryExist(itemF)) {
				// remove multiple '\\' at the end if there are...
				itemF = FWMFW::Utils::trimChars(itemF, [](int c) -> bool { return (c == '\\');  });

				// ensure that dir ends with "\\"
				if (!FWMFW::Utils::stringEndsWith(itemF, "\\")) itemF = itemF.append("\\");

				std::string::size_type startIdx = itemF.find_last_of('\\', itemF.find_last_of('\\') - 1) + 1;
				auto result = FWMFW::Utils::getFilesInDirectory(itemF, ".exe", true);
				for (auto&& f : result) {
					requestedFilesToBlock[f] = f.substr(startIdx);
				}
			}
			else if (FWMFW::Utils::doesFileExist(itemF)) {
				if (FWMFW::Utils::stringEndsWith(itemF, ".exe")) {
					// this basically gets the parent directory as the prefix for the rule name
					std::string::size_type startIdx = itemF.find_last_of('\\', itemF.find_last_of('\\') - 1) + 1;
					requestedFilesToBlock[itemF] = itemF.substr(startIdx);
				}
			}
		}

		// the text file should be parsed at this point
	
		std::unordered_map<std::string, std::string> filesToUnblock;

		// get all the existing rules created by this program
		FWMFW::WinNetFW::FireWallPolicy fwp;
		auto rules = fwp.getRules(
			[] (const std::string& name) -> bool {
				return (FWMFW::Utils::stringStartsWith(name, Constants::RULE_OUT_NAME_PREFIX));
			}
		);

		for (auto&& rule : rules) {
			// rule.first = key = appName
			const auto& appName = rule.first;
			// rule.second = value = ruleName
			const auto& ruleName = rule.second;

			if (requestedFilesToBlock.find(appName) != requestedFilesToBlock.end()) {
				// remove from request so the rule will not be added twice later
				requestedFilesToBlock.erase(appName);
			}
			else {
				// if not in the request, then this rule will be removed
				filesToUnblock[appName] = ruleName;
			}
		}

		std::size_t blockedCount = fwp.addBlockRules(
			requestedFilesToBlock,
			[] (const std::string& appName) -> void {
				std::cout << "Blocked: \"" << appName << "\"\n";
				return;
			}
		);
		std::cout.flush();

		std::size_t unblockedCount = fwp.removeBlockRules(
			filesToUnblock,
			[] (const std::string& appName) -> void {
				std::cout << "Unblocked: \"" << appName << "\"\n";
				return;
			}
		);
		std::cout.flush();

		std::cout << "Done. Blocked " << blockedCount << " files. Unblocked " << unblockedCount << " files." << std::endl;
	}
	catch (std::exception& e) {
		std::cerr << "An error has occurred: " << e.what() << "\n";
		return (-1);
	}
	catch (...) {
		std::cerr << "Unknown error.\n";
		return (-1);
	}

	FWMFW::WinNetFW::terminate();

	return (0);
}
