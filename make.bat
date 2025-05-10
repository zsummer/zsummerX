
echo "%cd%"

rem rd /Q /S build_win 
md build_win
cd build_win
cmake ../ -DCMAKE_BUILD_TYPE=Release -A x64 -DCMAKE_GENERATOR_PLATFORM=x64 
MSBuild zsummerX.sln /property:Configuration=Release /property:Platform=x64
xcopy ..\lib\Release\* ..\lib\ /Y
rd /Q /S  "../lib/Release" 
cmake ../ -DCMAKE_BUILD_TYPE=Debug  -A x64 -DCMAKE_GENERATOR_PLATFORM=x64
MSBuild zsummerX.sln /property:Configuration=Debug /property:Platform=x64
xcopy ..\lib\Debug\* ..\lib\ /Y
rd /Q /S  "../lib/Debug"
cd ..


md build_win_select
cd build_win_select
cmake ../ -DCMAKE_BUILD_TYPE=Release -DUSE_SELECT_IMPL=ON -A x64 -DCMAKE_GENERATOR_PLATFORM=x64 
MSBuild zsummerX.sln /property:Configuration=Release /property:Platform=x64
xcopy ..\lib\Release\* ..\lib\ /Y
rd /Q /S  "../lib/Release" 
cmake ../ -DCMAKE_BUILD_TYPE=Debug  -DUSE_SELECT_IMPL=ON -A x64 -DCMAKE_GENERATOR_PLATFORM=x64
MSBuild zsummerX.sln /property:Configuration=Debug /property:Platform=x64
xcopy ..\lib\Debug\* ..\lib\ /Y
rd /Q /S  "../lib/Debug"
cd ..



