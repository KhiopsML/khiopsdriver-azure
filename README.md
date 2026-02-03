khiopsdriver-azure
==================

[Khiops] driver to enable Azure cloud storage access.
*khiopsdriver-azure* is a cross-platform C++ library.
It is a layer above the C++ Azure storage SDK, exposing the storage access functions as a shared library conforming to the Khiops storage driver interface.


Features
--------

- Read/write access to Azure cloud storage blobs and shared files
- Read/write access to Azurite blob storage emulator
- Supports GNU/Linux (distributions using glibc only), macOS and Windows


Storage service authentication
------------------------------

The table below shows the supported methods of authentication.

| Authentication method         | Azurite storage emulator | Azure cloud storage |
| ----------------------------- | :----------------------: | :-----------------: |
| Connection string             | ✔                        | ✔                   |
| Environment credentials *     |                          | ✔                   |
| Workload identity credentials |                          | ✔                   |
| Managed identity credentials  |                          | ✔                   |
| Azure CLI credentials         |                          | ✔                   |

_* Client ID + client secret or certificate environment variables_


Distributions
-------------

This driver is distributed as multiple package formats:
- DEB package ([GitHub Actions workflow](.github/workflows/pack-debian.yml))
- RPM package ([GitHub Actions workflow](.github/workflows/pack-rpm.yml))
- pip package ([GitHub Actions workflow](.github/workflows/pack-pip.yml))
- conda package ([recipe repository](https://github.com/KhiopsML/recipe-khiopsdriver-azure))


Technical information
---------------------

This driver must be compiled with a compiler supporting at least C++14.

This repository also contains a test suite using GoogleTest.

The build tool used to compile the code into a library is CMake. Presets are available, using Ninja as the default generator.

The dependencies of this driver come from vcpkg, a revision of which is referenced by this repository as a Git submodule.


Other drivers
-------------

You may also want to use:
- Amazon S3 driver: https://github.com/KhiopsML/khiopsdriver-s3
- Google Cloud Storage driver: https://github.com/KhiopsML/khiopsdriver-gcs


[Khiops]: https://github.com/KhiopsML/khiops
