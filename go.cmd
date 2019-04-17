@echo off
cls
setlocal EnableDelayedExpansion

set vsVersion="16"
set buildFile=%~dp0msvc\build.build

set requires=Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64
set findMsb=MSBuild\**\Bin\amd64\MSBuild.exe

set vsWhereDir=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer
set vsWhereBaseCmd=.\vswhere.exe -latest -requires %requires% -version %vsVersion%

pushd "%vsWhereDir%" || echo vsWhere not found && exit /b 1
for /F "tokens=* usebackq delims=" %%i in (`%vsWhereBaseCmd% -find %findMsb%`) do set msbuild=%%i
for /F "tokens=* usebackq delims=" %%i in (`%vsWhereBaseCmd% -property installationPath`) do set vsInstallationPath=%%i
popd

if "%msbuild%" equ "" echo msbuild not found & exit /b 1
if "%vsInstallationPath%" equ "" echo visual studio not found & exit /b 1

echo msbuild=%msbuild%
echo vsInstallationPath=%vsInstallationPath%

set args=%*
if "%1" NEQ "" set args=/t:%*

call "%vsInstallationPath%\VC\Auxiliary\Build\vcvars64.bat"
echo include:
for %%A in ("%include:;=";"%") do (echo %%~A)

"%msbuild%" /p:vsInstallationPath="%vsInstallationPath%" %buildFile% %args%
exit /b 0