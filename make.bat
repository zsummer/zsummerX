cd depends/log4z
call make.bat
echo "%cd%"

xcopy .\lib\* ..\..\lib\ /Y
cd ../../
echo "%cd%"

rd /Q /S vs_sln 
md vs_sln
cd vs_sln
cmake ../ -DCMAKE_BUILD_TYPE=Release -A x64 -DCMAKE_GENERATOR_PLATFORM=x64 
MSBuild zsummerX.sln /property:Configuration=Release /property:Platform=x64
xcopy ..\lib\Release\* ..\lib\ /Y
rd /Q /S  "../lib/Release" 
cmake ../ -DCMAKE_BUILD_TYPE=Debug  -A x64 -DCMAKE_GENERATOR_PLATFORM=x64
MSBuild zsummerX.sln /property:Configuration=Debug /property:Platform=x64
xcopy ..\lib\Debug\* ..\lib\ /Y
rd /Q /S  "../lib/Debug"
cd ..