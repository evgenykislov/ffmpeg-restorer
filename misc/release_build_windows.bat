
set VER_INFO=

if "%1" == "" goto wo_version
set VER_INFO=-DAPPLICATION_VERSION=%1
:wo_version

pushd ..

rd /S /Q build-win-release
cmake -B build-win-release -DCMAKE_BUILD_TYPE=Release %VER_INFO%
cmake --build build-win-release --config Release

popd
