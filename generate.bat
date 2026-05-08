@echo off
setlocal
pushd "%~dp0"

where py >nul 2>nul
if not errorlevel 1 goto run_py

where python >nul 2>nul
if not errorlevel 1 goto run_python

echo Python launcher not found.
set "RESULT=1"
goto finish

:run_py
py -3 "%~dp0generate.py" %*
set "RESULT=%errorlevel%"
goto finish

:run_python
python "%~dp0generate.py" %*
set "RESULT=%errorlevel%"
goto finish

:finish
popd
echo.
if "%RESULT%"=="0" (
    echo generate.bat completed successfully.
) else (
    echo generate.bat failed. Exit code: %RESULT%
)
echo.
pause
exit /b %RESULT%
