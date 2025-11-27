@echo off
setlocal

echo ============================================
echo   Setting QNX SDP Environment
echo ============================================
call "C:\Users\Garraio\qnx8sdp\qnxsdp-env.bat"

echo ============================================
echo   Configuring CMake (if needed)
echo ============================================
cmake -B build -G "Unix Makefiles" ^
    -DCMAKE_C_COMPILER=qcc ^
    -DCMAKE_CXX_COMPILER=q++

echo ============================================
echo   Building Project
echo ============================================
cmake --build build -- -j8

echo ============================================
echo   Build Completed
echo ============================================
endlocal
