#include "WinNetFW.hxx"

#include <netfw.h>

#include <memory>

#include "Utils.hxx"

namespace FWMFW
{
	namespace WinNetFW
	{
		namespace
		{
			// A simple deleter for COM pointers stored within the std::unique_ptr class
			template<typename COMPtrType>
			struct ReleaseDeleter
			{
				void operator()(COMPtrType ptr) const
				{
					ptr->Release();
					return;
				}
			}; // struct ReleaseDeleter
		} // anonymous namespace

		bool initialize(void)
		{
			HRESULT result = CoInitialize(NULL);
			return (SUCCEEDED(result));
		}

		void terminate(void)
		{
			CoUninitialize();
			return;
		}

		FireWallPolicy::FireWallPolicy(void) : _implUnknown(nullptr), _implDispatch(nullptr)
		{
			HRESULT result = CoCreateInstance(CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*) &_implUnknown);

			if (FAILED(result)) {
				throw (Exception("FireWallPolicy: CoCreateInstance failed!"));
			}

			result = _implUnknown->QueryInterface(IID_IDispatch, (void**) &_implDispatch);

			if (FAILED(result)) {
				throw (Exception("FireWallPolicy: QueryInterface failed!"));
			}

			return;
		}

		FireWallPolicy::~FireWallPolicy(void)
		{
			if (_implDispatch != nullptr) {
				_implDispatch->Release();
				_implDispatch = nullptr;
			}
			if (_implUnknown != nullptr) {
				_implUnknown->Release();
				_implUnknown = nullptr;
			}
			return;
		}

		std::unordered_map<std::string, std::string> FireWallPolicy::getRules(const FireWallPolicy::FilterFunction&& ruleNameFilter, const FireWallPolicy::FilterFunction&& appNameFilter) const
		{
			std::unordered_map<std::string, std::string> result;
			INetFwPolicy2* fwPolicy = nullptr;

			try {
				fwPolicy = static_cast<INetFwPolicy2*>(_implDispatch);

				INetFwRules* fwRulesTmp = nullptr;
				fwPolicy->get_Rules(&fwRulesTmp);

				typedef ReleaseDeleter<INetFwRules*> FWRulesDeleter;
				std::unique_ptr<INetFwRules, FWRulesDeleter> fwRules(fwRulesTmp, FWRulesDeleter());

				IEnumVARIANT* elemsTmp = nullptr;
				fwRules->get__NewEnum(reinterpret_cast<IUnknown**>(&elemsTmp));

				typedef ReleaseDeleter<IEnumVARIANT*> EnumVARIANTDeleter;
				std::unique_ptr<IEnumVARIANT, EnumVARIANTDeleter> elems(elemsTmp, EnumVARIANTDeleter());

				long ruleCount = 0;
				fwRules->get_Count(&ruleCount);

				for (long ctr = 1; ctr <= ruleCount; ctr++) {
					VARIANT vResult[1];
					elems->Next(1, vResult, NULL);

					typedef ReleaseDeleter<INetFwRule*> FWRuleDeleter;
					std::unique_ptr<INetFwRule, FWRuleDeleter> fwRule(static_cast<INetFwRule*>(vResult[0].pdispVal), FWRuleDeleter());

					BSTR name;
					BSTR applicationName;
					fwRule->get_Name(&name);
					fwRule->get_ApplicationName(&applicationName);

					// ignore rules that have empty names
					if (SysStringLen(name) > 0 && SysStringLen(applicationName) > 0) {
						auto ruleName = Utils::w32WStrToUTF8Str(name);
						auto appName = Utils::w32WStrToUTF8Str(applicationName);

						bool isMatch = true;
						if (ruleNameFilter != nullptr) {
							isMatch = isMatch && ruleNameFilter(ruleName);
						}
						if (appNameFilter != nullptr) {
							isMatch = isMatch && appNameFilter(ruleName);
						}

						if (isMatch) {
							result[appName] = ruleName;
						}
					}
				}

				return (result);
			}
			catch (std::exception& e) {
				std::string msg("Error getting rules: ");
				msg = msg.append(e.what());
				throw (Exception(msg));
			}
			catch (...) {
				throw (Exception("An unknown error has occurred."));
			}
		}

		std::size_t FireWallPolicy::addBlockRules(const std::unordered_map<std::string, std::string>& rules, const RuleChangedCallback&& fileBlockAddedCallback)
		{
			std::size_t result = 0;
			INetFwPolicy2* fwPolicy = nullptr;

			try {
				fwPolicy = static_cast<INetFwPolicy2*>(_implDispatch);

				INetFwRules* fwRulesTmp = nullptr;
				fwPolicy->get_Rules(&fwRulesTmp);

				typedef ReleaseDeleter<INetFwRules*> FWRulesDeleter;
				std::unique_ptr<INetFwRules, FWRulesDeleter> fwRules(fwRulesTmp, FWRulesDeleter());

				HRESULT hResult = ERROR_SUCCESS;
				for (auto&& rule : rules) {
					const auto& appName = rule.first;
					const auto& ruleName = rule.second;

					// IN and OUT rules
					std::unordered_map<NET_FW_RULE_DIRECTION, bool> directions;
					directions[NET_FW_RULE_DIR_IN] = false;
					directions[NET_FW_RULE_DIR_OUT] = false;

					for (auto&& direction : directions) {
						const auto& fwRuleDirection = direction.first;
						IUnknown* fwRuleUnknownTmp = nullptr;
						hResult = CoCreateInstance(CLSID_NetFwRule, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*) &fwRuleUnknownTmp);
						if (FAILED(hResult)) {
							throw (Exception("CoCreateInstance failed."));
						}

						typedef ReleaseDeleter<IUnknown*> UnknownDeleter;
						std::unique_ptr<IUnknown, UnknownDeleter> fwRuleUnkown(fwRuleUnknownTmp, UnknownDeleter());

						IDispatch* fwRuleDispatchTmp = nullptr;
						hResult = fwRuleUnkown->QueryInterface(IID_IDispatch, (void**) &fwRuleDispatchTmp);
						if (FAILED(hResult)) {
							throw (Exception("QueryInterface failed."));
						}

						typedef ReleaseDeleter<IDispatch*> DispatchDeleter;
						std::unique_ptr<IDispatch, DispatchDeleter> fwRuleDispatch(fwRuleDispatchTmp, DispatchDeleter());

						// just a weak pointer
						INetFwRule* fwRule = static_cast<INetFwRule*>(fwRuleDispatch.get());

						// TODO: make this more generic, and remove any references to FMWFW.
						fwRule->put_Action(NET_FW_ACTION_BLOCK);
						fwRule->put_ApplicationName(const_cast<BSTR>(Utils::utf8StrToW32WStr(appName).c_str()));
						fwRule->put_Description(OLESTR("Blocked using FMWFW."));
						fwRule->put_Direction(fwRuleDirection);
						fwRule->put_Enabled(VARIANT_TRUE);

						std::wstring ruleNameWStr;
						if (fwRuleDirection == NET_FW_RULE_DIR_IN) {
							ruleNameWStr = std::wstring(L"FWMFW_IN_") + Utils::utf8StrToW32WStr(ruleName);
						}
						else {
							ruleNameWStr = std::wstring(L"FWMFW_OUT_") + Utils::utf8StrToW32WStr(ruleName);
						}
						fwRule->put_Name(const_cast<BSTR>(ruleNameWStr.c_str()));

						hResult = fwRules->Add(fwRule);
						directions[fwRuleDirection] = SUCCEEDED(hResult);
						if (directions[NET_FW_RULE_DIR_IN] && directions[NET_FW_RULE_DIR_OUT]) {
							result++;
							if (fileBlockAddedCallback != nullptr) {
								fileBlockAddedCallback(appName);
							}
						}
					}
				}
			}
			catch (std::exception& e) {
				std::string msg("Error adding rules: ");
				msg = msg.append(e.what());
				throw (Exception(msg));
			}
			catch (...) {
				throw (Exception("An unknown error has occurred."));
			}
			return (result);
		}

		std::size_t FireWallPolicy::removeBlockRules(const std::unordered_map<std::string, std::string>& rules, const RuleChangedCallback&& fileBlockRemovedCallback)
		{
			std::size_t result = 0;
			INetFwPolicy2* fwPolicy = nullptr;

			try {
				fwPolicy = static_cast<INetFwPolicy2*>(_implDispatch);

				INetFwRules* fwRulesTmp = nullptr;
				fwPolicy->get_Rules(&fwRulesTmp);

				typedef ReleaseDeleter<INetFwRules*> FWRulesDeleter;
				std::unique_ptr<INetFwRules, FWRulesDeleter> fwRules(fwRulesTmp, FWRulesDeleter());

				for (auto&& rule : rules) {
					const auto& appName = rule.first;
					const auto& ruleName = rule.second;

					// IN and OUT rules
					std::unordered_map<NET_FW_RULE_DIRECTION, bool> directions;
					directions[NET_FW_RULE_DIR_IN] = false;
					directions[NET_FW_RULE_DIR_OUT] = false;

					for (auto&& direction : directions) {
						const auto& fwRuleDirection = direction.first;

						// TODO: Remove assumption that the ruleName has default prefix of "FWMFW_OUT_"
						std::wstring ruleNameWStr = Utils::utf8StrToW32WStr(ruleName);
						if (fwRuleDirection == NET_FW_RULE_DIR_IN) {
							// change "FWMFW_OUT_" to "FWMFW_IN_"
							ruleNameWStr = std::wstring(L"FWMFW_IN_") + ruleNameWStr.substr(wcsnlen_s(L"FWMFW_OUT_", 10));
						}

						HRESULT hResult = fwRules->Remove(const_cast<BSTR>(ruleNameWStr.c_str()));

						directions[fwRuleDirection] = SUCCEEDED(hResult);
						if (directions[NET_FW_RULE_DIR_IN] && directions[NET_FW_RULE_DIR_OUT]) {
							result++;
							if (fileBlockRemovedCallback != nullptr) {
								fileBlockRemovedCallback(appName);
							}
						}

					}
				}
			}
			catch (std::exception& e) {
				std::string msg("Error removing rules: ");
				msg = msg.append(e.what());
				throw (Exception(msg));
			}
			catch (...) {
				throw (Exception("An unknown error has occurred."));
			}

			return (result);
		}

	} // namespace WinNetFW
} // namespace FWMFW
