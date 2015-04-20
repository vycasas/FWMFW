#if !defined(DOTSLASHZERO_FWMFW_WINNETFW_HXX)
#define DOTSLASHZERO_FWMFW_WINNETFW_HXX

#include <Windows.h>

#include <exception>
#include <functional>
#include <string>
#include <unordered_map>

// set of wrapper classes using bridge pattern (for RAII)

namespace FWMFW
{
    namespace WinNetFW
    {
        // start up/tear down routines - these functions must be called prior to any operations within this namespace
        bool initialize(void);
        void terminate(void);

        class Exception : std::exception
        {
        public:
            Exception(void) : std::exception(), _message("")
            { return; }

            Exception(const std::string& message) : std::exception(), _message(message)
            { return; }

            std::string getMessage(void) const { return (_message); }

            virtual const char* what(void) const /*noexcept override*/ { return (_message.c_str()); }

        private:
            std::string _message;
        }; // class Exception

        class FireWallPolicy
        {
        public:
            FireWallPolicy(void);
            ~FireWallPolicy(void);

            typedef std::function<bool(const std::string&)> FilterFunction;

            // TODO: attach more information to the return data (e.g. blocked/allowed, enabled, etc.)
            std::unordered_map<std::string, std::string> getRules(
                const FilterFunction&& ruleNameFilter = nullptr,
                const FilterFunction&& appNameFilter = nullptr
            ) const;

            typedef std::function<void(const std::string&)> RuleChangedCallback;

            // technically, these two functions does not modify the class itself
            // (as well as the two pointer data members) so they can be marked as "const"
            // arg: map<appName, ruleName>
            std::size_t addBlockRules(
                const std::unordered_map<std::string, std::string>& rules,
                const RuleChangedCallback&& fileBlockAddedCallback = nullptr
            );
            std::size_t removeBlockRules(
                const std::unordered_map<std::string, std::string>& rules,
                const RuleChangedCallback&& fileBlockRemovedCallback = nullptr
            );

            FireWallPolicy(const FireWallPolicy&) = delete;
            FireWallPolicy(const FireWallPolicy&&) = delete;
            FireWallPolicy& operator=(const FireWallPolicy&) = delete;
            FireWallPolicy& operator=(const FireWallPolicy&&) = delete;
        private:
            IUnknown* _implUnknown;
            IDispatch* _implDispatch;
        }; // class FireWallPolicy
    } // namespace WinNetFW
} // namespace FWMFW

#endif // !defined(DOTSLASHZERO_FWMFW_WINNETFW_HXX)
