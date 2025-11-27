@echo off
setlocal

echo ============================================
echo   Setting QNX SDP Environment
echo ============================================
call "C:\Users\Garraio\qnx8sdp\qnxsdp-env.bat"

echo ============================================
echo   Removing Old build directory
echo ============================================
rmdir /s /q build 2>nul

echo ============================================
echo   Configuring CMake
echo ============================================
cmake -B build -G "Unix Makefiles" ^
    -DCMAKE_C_COMPILER=qcc ^
    -DCMAKE_CXX_COMPILER=q++

echo ============================================
echo   Building Project (Full Rebuild)
echo ============================================
cmake --build build -- VERBOSE=1 -- -j8

echo ============================================
echo   Full Build Completed
echo ============================================
endlocal
