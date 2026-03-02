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
#include <azure/core/datetime.hpp>
#include <azure/core/credentials/credentials.hpp>
#include <azure/storage/blobs/blob_sas_builder.hpp>
#include <azure/storage/files/shares/share_sas_builder.hpp>
#include <chrono>
#include <sstream>
#include <string>

namespace az {

struct Auth {
    std::string sUriAuth; // Will also contain the authentication token if not in the HTTP header, which is the case when using a connection string.
    std::string sAuthHeader; // Used only when not using a connection string.
    bool HasHeader() const {return !sAuthHeader.empty();}
};

Auth BuildAuth(const ServiceRequest &request) {
    if (request.bUsingConnectionString) {
        std::string sToken;
        if (request.storageType == BLOB) {
            Azure::Storage::Sas::BlobSasBuilder sasbuilder;
            sasbuilder.BlobContainerName = request.blob.sContainer;
            sasbuilder.BlobName = request.blob.sBlob;
            sasbuilder.Resource = Azure::Storage::Sas::BlobSasResource::Blob;
            sasbuilder.ExpiresOn = Azure::DateTime::clock::now() + std::chrono::hours(2);
            sasbuilder.SetPermissions(Azure::Storage::Sas::BlobSasPermissions::Read);
            sToken = sasbuilder.GenerateSasToken(*request.connectionStringCredential);
        } else /* SHARE */ {
            Azure::Storage::Sas::ShareSasBuilder sasbuilder;
            sasbuilder.ShareName = request.share.sShare;
            auto pathvec = request.share.path;
            std::ostringstream oss;
            for (std::string sSegment : pathvec) {
                oss << sSegment;
            }
            sasbuilder.FilePath = oss.str();
            sasbuilder.Resource = Azure::Storage::Sas::ShareSasResource::File;
            sasbuilder.ExpiresOn = Azure::DateTime::clock::now() + std::chrono::hours(2);
            sasbuilder.SetPermissions(Azure::Storage::Sas::ShareSasPermissions::Read);
            sToken = sasbuilder.GenerateSasToken(*request.connectionStringCredential);
        }
        return {request.azureUrl.GetAbsoluteUrl() + "?" + sToken, ""};
    } else /* using chained key credential */ {
        auto token = request.noConnectionStringCredential->GetToken(
            Azure::Core::Credentials::TokenRequestContext{{"https://storage.azure.com/.default"}},
            Azure::Core::Context()
        );

        return {request.azureUrl.GetAbsoluteUrl(), std::string("Bearer ") + token.Token};
    }
}

}
