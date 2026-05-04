@echo off
setlocal

where py >nul 2>nul
if not errorlevel 1 goto run_py

where python >nul 2>nul
if not errorlevel 1 goto run_python

echo Python launcher not found.
exit /b 1

:run_py
py -3 "%~dp0generate.py" %*
exit /b %errorlevel%

:run_python
python "%~dp0generate.py" %*
exit /b %errorlevel%
