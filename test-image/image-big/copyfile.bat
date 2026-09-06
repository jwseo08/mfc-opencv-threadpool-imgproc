@echo off
setlocal EnableDelayedExpansion

for /L %%B in (1,1,9) do (
    for /L %%I in (1,1,10) do (
        set /A DEST=%%B*10+%%I

        set "SRC=0000%%I"
        set "SRC=!SRC:~-4!"

        set "DST=0000!DEST!"
        set "DST=!DST:~-4!"

        copy /Y "!SRC!.jpg" "!DST!.jpg" >nul

        echo !SRC!.jpg ^-^> !DST!.jpg
    )
)

echo.
echo done
pause