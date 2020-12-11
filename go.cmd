@echo off & cls & setlocal

::[minInclude, maxExclude)
set vsVersion=[16^^^^,17^^^^)
set buildFile=%~dp0msvc\build.build
set requires=Microsoft.Component.MSBuild^
 Microsoft.VisualStudio.Component.VC.Tools.x86.x64^
 Microsoft.VisualStudio.Component.VC.CMake.Project

set vsWhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %vswhere% echo missing vswhere & exit /b 1
set vsWhereBaseCmd=%vsWhere% -products * -latest -version %vsVersion% -requires %requires% 
for /f "tokens=* usebackq delims=" %%i in (`%vsWhereBaseCmd% -property installationPath`) do set vsInstallationPath=%%i
if "%vsInstallationPath%" equ "" echo Visual studio not found & exit /b 1
echo vsInstallationPath = %vsInstallationPath%

for /f "tokens=* usebackq delims=" %%i in (`%vsWhereBaseCmd% -find MSBuild\**\Bin\amd64\MSBuild.exe`) do set msbuild=%%i
if "%msbuild%" equ "" echo msbuild not found & exit /b 1
echo msbuild=%msbuild%

(set __VSCMD_ARG_NO_LOGO=1 && call "%vsInstallationPath%\VC\Auxiliary\Build\vcvars64.bat") || (echo vcvars64 failed: %errorlevel% & exit /b 1)

set "args=%*"
set "switch=0"
if not defined args set args=-nologo
for %%a in (^- ^/) do if "%args:~0,1%"=="%%a" set "switch=1"
if "%switch%" equ "0" (set "args=/t:%args%")

echo "%msbuild%" %buildFile% %args%
"%msbuild%" %buildFile% %args%