// The return values of the functions exposed by the driver library.

#pragma once

namespace az {
constexpr int nSuccess = 1;
constexpr int nFailure = 0;
constexpr int nTrue = 1;
constexpr int nFalse = 0;
constexpr long long int nSizeFailure = -1;
constexpr int nCloseSuccess = 0;
constexpr int nCloseFailure = -1;
constexpr int nReadFailure = -1;
constexpr int nSeekSuccess = 0;
constexpr int nSeekFailure = -1;
constexpr int nWriteFailure = -1;
constexpr int nFlushSuccess = 0;
constexpr int nFlushFailure = -1;
constexpr int nFreeDiskSpaceFailure = -1;
constexpr int nGenericFailure = -1;
} // namespace az
