#!/bin/bash

# Set-up the shell to behave more like a general-purpose programming language
set -euo pipefail

# Configure project
cmake --fresh -G Ninja -D CMAKE_BUILD_TYPE=Release -D AZURE_PLUGIN_BUILD_ENV=conda -B builds/conda -S .

# Build
cmake --build builds/conda --target khiopsdriver_file_azure

# Copy binary to conda package
cmake --install builds/conda --prefix $PREFIX




