@echo off
if "%~1"=="" (
    echo Usage: from-yt.bat youtube_url
    echo Example: from-yt.bat "https://www.youtube.com/watch?v=dQw4w9WgXcQ"
    exit /b 1
)

set YOUTUBE_URL=%~1
set TEMP_AUDIO=temp_audio
set TEMP_WAV=temp_audio.wav
set PROCESSED_WAV=temp_audio_processed.wav

echo Starting YouTube to RS485 conversion...
echo URL: %YOUTUBE_URL%
echo.

echo Step 1/4: Downloading audio from YouTube...
yt-dlp -x "%YOUTUBE_URL%" -o "%TEMP_AUDIO%.%%(ext)s"
if %errorlevel% neq 0 (
    echo Error downloading from YouTube
    exit /b 1
)

echo.
echo Step 2/4: Converting to WAV format...
for %%f in ("%TEMP_AUDIO%.*") do (
    if not "%%~xf"==".wav" (
        ffmpeg.exe -i "%%f" -y "%TEMP_WAV%"
        del "%%f"
    ) else (
        ren "%%f" "%TEMP_WAV%"
    )
)

if not exist "%TEMP_WAV%" (
    echo Error: Could not create WAV file
    exit /b 1
)

echo.
echo Step 3/4: Processing audio with process.bat...
call process.bat "%TEMP_WAV%"
if %errorlevel% neq 0 (
    echo Error processing WAV file
    goto cleanup
)

echo.
echo Step 4/4: Running wav_parser.exe...
wav_parser.exe "%PROCESSED_WAV%"
if %errorlevel% neq 0 (
    echo Error running wav_parser.exe
    goto cleanup
)

echo.
echo Success! YouTube audio processed and parsed.


:cleanup
if exist "%TEMP_WAV%" del "%TEMP_WAV%"
if exist "%PROCESSED_WAV%" del "%PROCESSED_WAV%"
echo Cleaned up temporary files.