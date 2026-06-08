/*
As most of the functions in the driver directly authenticate to Azure cloud
using the various blob / file / directory / ... clients, this file is only
for authenticating when we are not acting directly through a client.

For example, the concatenation function acts directly through a client for
the destination object but the sources passed to the Azure SDK functions are
merely URL strings. Thus we must be correctly authenticated for the sources.
*/

#pragma once

#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_azure/util.hpp"
#include <azure/core/credentials/credentials.hpp>
#include <azure/core/datetime.hpp>
#include <azure/storage/blobs/blob_sas_builder.hpp>
#include <azure/storage/files/shares/share_sas_builder.hpp>
#include <chrono>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace khiops_driver_azure {

struct Auth {
  std::string
      sUriAuth; // Will also contain the authentication token if not in the HTTP
                // header, which is the case when using a connection string.
  std::string sAuthHeader; // Used only when not using a connection string.
  bool HasHeader() const { return !sAuthHeader.empty(); }
};

static int BuildAuth(Auth *result, const ServiceRequest &request) {
  khiops_driver_common::GetLogger()->debug("Building authentication object...");
  khiops_driver_common::GetLogger()->debug("  Using connection string: {}",
                              request.is_using_connection_string ? "true"
                                                             : "false");
  khiops_driver_common::GetLogger()->debug("  Storage type: {}",
                              request.storage_type == BLOB ? "BLOB" : "FILE");
  khiops_driver_common::GetLogger()->debug("  URL: {}", request.azure_url.GetAbsoluteUrl());
  if (request.is_using_connection_string) {
    std::string sToken;
    if (request.storage_type == BLOB) {
      Azure::Storage::Sas::BlobSasBuilder sasbuilder;
      sasbuilder.BlobContainerName = *request.object_path.blob_container;
      sasbuilder.BlobName = *request.object_path.blob;
      sasbuilder.Resource = Azure::Storage::Sas::BlobSasResource::Blob;
      sasbuilder.StartsOn =
          Azure::DateTime::clock::now() - std::chrono::minutes(5);
      sasbuilder.ExpiresOn =
          Azure::DateTime::clock::now() + std::chrono::hours(2);
      sasbuilder.SetPermissions(Azure::Storage::Sas::BlobSasPermissions::Read);
      khiops_driver_common::GetLogger()->debug("  BLOB SAS builder:");
      khiops_driver_common::GetLogger()->debug("    BLOB container name: {}",
                                  sasbuilder.BlobContainerName);
      khiops_driver_common::GetLogger()->debug("    BLOB name: {}", sasbuilder.BlobName);
      sToken = sasbuilder.GenerateSasToken(*request.connection_string_credential);
    } else /* SHARE */ {
      Azure::Storage::Sas::ShareSasBuilder sasbuilder;
      sasbuilder.ShareName = *request.object_path.file_share;
      std::vector<std::string> pathSegments = *request.object_path.file_path;
      if (pathSegments.empty()) {
        khiops_driver_common::GetLogger()->error("Shared file path is empty.");
        return -1;
      }
      std::ostringstream oss;
      oss << pathSegments[0];
      for (size_t i = 1ULL; i < pathSegments.size(); i++) {
        oss << "/" << pathSegments[i];
      }
      sasbuilder.FilePath = oss.str();
      sasbuilder.Resource = Azure::Storage::Sas::ShareSasResource::File;
      sasbuilder.StartsOn =
          Azure::DateTime::clock::now() - std::chrono::minutes(5);
      sasbuilder.ExpiresOn =
          Azure::DateTime::clock::now() + std::chrono::hours(2);
      sasbuilder.SetPermissions(Azure::Storage::Sas::ShareSasPermissions::Read);
      khiops_driver_common::GetLogger()->debug("  SHARE SAS builder:");
      khiops_driver_common::GetLogger()->debug("    SHARE name: {}", sasbuilder.ShareName);
      khiops_driver_common::GetLogger()->debug("    FILE path: {}", sasbuilder.FilePath);
      sToken = sasbuilder.GenerateSasToken(*request.connection_string_credential);
    }
    *result = {request.azure_url.GetAbsoluteUrl() + "?" + sToken, ""};
  } else /* using chained key credential */ {
    Azure::Core::Credentials::TokenRequestContext trc;
    trc.Scopes = {"https://storage.azure.com/.default"};
    auto token = request.no_connection_string_credential->GetToken(
        trc, Azure::Core::Context());

    *result = {request.azure_url.GetAbsoluteUrl(),
               std::string("Bearer ") + token.Token};
  }
  return 0;
}

} // namespace khiops_driver_azure
