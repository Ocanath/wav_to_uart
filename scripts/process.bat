@echo off
if "%~1"=="" (
    echo Usage: process.bat input_file
    echo Example: process.bat input.wav
    exit /b 1
)

set INPUT_FILE=%~1
set OUTPUT_FILE=%~n1_processed%~x1

echo Processing %INPUT_FILE%...
echo - Downsampling to 23040Hz
echo - Applying highpass filter at 1000Hz
echo - Applying lowpass filter at 10000Hz  
echo - Scaling volume by 2000/32767 (0.061)

ffmpeg.exe -i "%INPUT_FILE%" -ar 18000 -ac 1 -af "highpass=f=100,lowpass=f=18000,volume=2000/32767" -y "%OUTPUT_FILE%"

if %errorlevel% equ 0 (
    echo Success! Output saved as: %OUTPUT_FILE%
) else (
    echo Error processing file
    exit /b 1
)