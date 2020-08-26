set dest=%1%
for /f %%a in ('git rev-parse --short HEAD') do set COMMIT=%%a
echo #define BUILD_SUFFIX %COMMIT% >%dest%build.h.tmp
fc "%dest%build.h" "%dest%build.h.tmp" >nul
if %errorlevel% NEQ 0 echo #define BUILD_SUFFIX %COMMIT% >%dest%build.h