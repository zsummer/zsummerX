
xcopy depends\\log4z include\\log4z /s /y  /i
xcopy depends\\proto4z include\\proto4z /s /y  /i
xcopy depends\\rc4 include\\rc4 /s /y  /i
xcopy depends\\lua include\\lua /s /y  /i
xcopy depends\\log4z\\lib lib /s /y  /i

set destPath=Z:\\github\\breeze

xcopy include %destPath%\\depends\\include /s /y  /i

xcopy lib %destPath%\\depends_win\\lib /s /y  /i
xcopy lib %destPath%\\depends_linux\\lib /s /y  /i

pause