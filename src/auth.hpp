/*
As most of the functions in the driver directly authenticate to Azure cloud
using the various blob / file / directory / ... clients, this file is only
for authenticating when we are not acting directly through a client.

For example, the concatenation function acts directly through a client for
the destination object but the sources passed to the Azure SDK functions are
merely URL strings. Thus we must be correctly authenticated for the sources.
*/

#pragma once

#include "servicerequest.hpp"
#include "logging.hpp"
#include <azure/core/datetime.hpp>
#include <azure/core/credentials/credentials.hpp>
#include <azure/storage/blobs/blob_sas_builder.hpp>
#include <azure/storage/files/shares/share_sas_builder.hpp>
#include <chrono>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace az {

struct Auth {
    std::string sUriAuth; // Will also contain the authentication token if not in the HTTP header, which is the case when using a connection string.
    std::string sAuthHeader; // Used only when not using a connection string.
    bool HasHeader() const {return !sAuthHeader.empty();}
};

static int BuildAuth(Auth *result, const ServiceRequest &request) {
    logging::getLogger()->debug("Building authentication object...");
    logging::getLogger()->debug("  Using connection string: {}", request.bUsingConnectionString ? "true": "false");
    logging::getLogger()->debug("  Storage type: {}", request.storageType == BLOB ? "BLOB" : "FILE");
    logging::getLogger()->debug("  URL: {}", request.azureUrl.GetAbsoluteUrl());
    if (request.bUsingConnectionString) {
        std::string sToken;
        if (request.storageType == BLOB) {
            Azure::Storage::Sas::BlobSasBuilder sasbuilder;
            sasbuilder.BlobContainerName = request.blob.sContainer;
            sasbuilder.BlobName = request.blob.sBlob;
            sasbuilder.Resource = Azure::Storage::Sas::BlobSasResource::Blob;
            sasbuilder.StartsOn = Azure::DateTime::clock::now() - std::chrono::minutes(5);
            sasbuilder.ExpiresOn = Azure::DateTime::clock::now() + std::chrono::hours(2);
            sasbuilder.SetPermissions(Azure::Storage::Sas::BlobSasPermissions::Read);
            logging::getLogger()->debug("  BLOB SAS builder:");
            logging::getLogger()->debug("    BLOB container name: {}", sasbuilder.BlobContainerName);
            logging::getLogger()->debug("    BLOB name: {}", sasbuilder.BlobName);
            sToken = sasbuilder.GenerateSasToken(*request.connectionStringCredential);
        } else /* SHARE */ {
            Azure::Storage::Sas::ShareSasBuilder sasbuilder;
            sasbuilder.ShareName = request.share.sShare;
            std::vector<std::string> pathSegments = request.share.path;
            if(pathSegments.empty()) {
                logging::getLogger()->error("Shared file path is empty.");
                return -1;
            }
            std::ostringstream oss;
            oss << pathSegments[0];
            for (size_t i = 1ULL; i < pathSegments.size(); i++) {
                oss << "/" << pathSegments[i];
            }
            sasbuilder.FilePath = oss.str();
            sasbuilder.Resource = Azure::Storage::Sas::ShareSasResource::File;
            sasbuilder.StartsOn = Azure::DateTime::clock::now() - std::chrono::minutes(5);
            sasbuilder.ExpiresOn = Azure::DateTime::clock::now() + std::chrono::hours(2);
            sasbuilder.SetPermissions(Azure::Storage::Sas::ShareSasPermissions::Read);
            logging::getLogger()->debug("  SHARE SAS builder:");
            logging::getLogger()->debug("    SHARE name: {}", sasbuilder.ShareName);
            logging::getLogger()->debug("    FILE path: {}", sasbuilder.FilePath);
            sToken = sasbuilder.GenerateSasToken(*request.connectionStringCredential);
        }
        *result = {request.azureUrl.GetAbsoluteUrl() + "?" + sToken, ""};
    } else /* using chained key credential */ {
        Azure::Core::Credentials::TokenRequestContext trc;
        trc.Scopes = {"https://storage.azure.com/.default"};
        auto token = request.noConnectionStringCredential->GetToken(trc, Azure::Core::Context());

        *result = {request.azureUrl.GetAbsoluteUrl(), std::string("Bearer ") + token.Token};
    }
    return 0;
}

}
