@echo off
cls
setlocal EnableDelayedExpansion

set version="[15,16.1)"

set buildFile=%~dp0msvc\build.build
set requires=Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64
set msbuild=MSBuild\**\Bin\amd64\MSBuild.exe

set vsWhereBaseCmd=.\vswhere.exe -latest -requires %requires% -version %version%

pushd "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"

for /F "tokens=* usebackq delims=" %%i in (`%vsWhereBaseCmd% -find %msbuild%`) do set msbuild=%%i
if "%msbuild%" equ "" echo msbuild not found & exit /b 1
echo msbuild=%msbuild%

for /F "tokens=* usebackq delims=" %%i in (`%vsWhereBaseCmd% -property installationPath`) do set vsInstallationPath=%%i
if "%vsInstallationPath%" equ "" echo visual studio not found & exit /b 1
echo vsInstallationPath=%vsInstallationPath%

set args=%*
if "%1" NEQ "" set args=/t:%*

"%msbuild%" /p:vsInstallationPath="%vsInstallationPath%" %buildFile% %args%

exit /b 0
