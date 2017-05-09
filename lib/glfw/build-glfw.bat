
if not defined DevEnvDir (
    CALL "E:\Programs\vs2017\VC\Auxiliary\Build\vcvars64.bat"
)

if not exist build mkdir build
pushd build
cmake -G "Visual Studio 15 2017 Win64" -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF ..
msbuild glfw.sln
xcopy src\Debug\glfw3.lib ..\lib\ /i /y

popd

