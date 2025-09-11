#pragma once

#include <vector>
#include <string>
#include <azure/core/url.hpp>
#include "exception.hpp"

namespace az
{
    namespace util
    {
        namespace str
        {
            std::vector<std::string> Split(const std::string& str, char delim, long long int nMaxSplits = -1, bool bRemoveEmpty = false);
            bool StartsWith(const std::string& str, const std::string& prefix);
            bool EndsWith(const std::string& str, const std::string& suffix);
            std::string ToLower(const std::string& str);
        }

        namespace random
        {
            bool RandomBool();
        }

        namespace env
        {
            class EnvironmentVariableNotFoundError : public Error
            {
            public:
                EnvironmentVariableNotFoundError(const std::string& sVarName);
                virtual const char* what() const noexcept override;

            private:
                std::string sMessage;
            };

            std::string GetEnvironmentVariableOrThrow(const std::string& sVarName);
            std::string GetEnvironmentVariableOrDefault(const std::string& sVarName, const std::string& sDefaultValue);
        }

        namespace connstr
        {
            class ParsingError : public Error
            {
            public:
                ParsingError(const std::string& sMessage);
                virtual const char* what() const noexcept override;

            private:
                std::string sMessage;
            };

            struct ConnectionString
            {
                std::string sAccountName;
                std::string sAccountKey;
                Azure::Core::Url blobEndpoint;

                static ConnectionString ParseConnectionString(const std::string& sConnectionString);

                friend bool operator==(const ConnectionString& a, const ConnectionString& b);
            };
        }

        namespace glob
        {
            size_t FindGlobbingChar(const std::string& str);
        }
    }
}
