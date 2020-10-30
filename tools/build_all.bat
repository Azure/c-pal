REM this file, when run from cmake folder, will re-re-re-re-re build the project in all combinations of GBALLOC_HL_TYPE and GBALLOC_LL_TYPE and run all tests for debug and release.
REM very useful on dev machine

cmake . -DGBALLOC_HL_TYPE:STRING=PASSTHROUGH -DGBALLOC_LL_TYPE:STRING=PASSTHROUGH
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=RelWithDebInfo /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=Debug /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr

cmake . -DGBALLOC_HL_TYPE:STRING=PASSTHROUGH -DGBALLOC_LL_TYPE:STRING=WIN32HEAP
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=RelWithDebInfo /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=Debug /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr

cmake . -DGBALLOC_HL_TYPE:STRING=PASSTHROUGH -DGBALLOC_LL_TYPE:STRING=MIMALLOC
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=RelWithDebInfo /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=Debug /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr

cmake . -DGBALLOC_HL_TYPE:STRING=METRICS -DGBALLOC_LL_TYPE:STRING=PASSTHROUGH
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=RelWithDebInfo /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=Debug /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr

cmake . -DGBALLOC_HL_TYPE:STRING=METRICS -DGBALLOC_LL_TYPE:STRING=WIN32HEAP
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=RelWithDebInfo /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=Debug /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr

cmake . -DGBALLOC_HL_TYPE:STRING=METRICS -DGBALLOC_LL_TYPE:STRING=MIMALLOC
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=RelWithDebInfo /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr
msbuild /t:restore /t:build /p:Configuration=Debug /m c_pal.sln
IF %ERRORLEVEL% NEQ 0 goto myerr
ctest -C "RelWithDebInfo" -j 128
IF %ERRORLEVEL% NEQ 0 goto myerr

goto noerr
:myerr
ECHO ERROR happened
goto myexit
:noerr
ECHO NO ERROR happened hurray!

:myexit