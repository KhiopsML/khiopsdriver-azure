REM Echo all output
@echo on

REM We need to use the subst command to shorten the paths:  
REM the buildsystem uses very long paths and may fail on your system. 
REM We recommend moving vcpkg to a short path such as 'C:\src\vcpkg' or using the subst command."
subst W: %CD% 
W:

REM Configure project
cmake --fresh -G Ninja -D CMAKE_BUILD_TYPE=Release -D AZURE_PLUGIN_BUILD_ENV=conda -B builds\conda -S .

REM Build
cmake --build builds\conda --parallel --target khiopsdriver_file_azure

REM Create drivers installation directory
mkdir %PREFIX%\bin
mkdir %PREFIX%\lib

REM Copy the libs for the driver package
cmake --install builds\conda --prefix %PREFIX%
