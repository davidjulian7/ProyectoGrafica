@echo off
setlocal
cd /d "%~dp0src"
echo ============================================
echo   Verificando dependencias del Proyecto Final
echo ============================================
set FALTAN=0
echo Bibliotecas (.lib) para compilar en Debug x64:
call :revisa "deps\glfw\MSVC2022\lib\x64\Debug\glfw3.lib"
call :revisa "deps\glad\MSVC2022\lib\x64\Debug\glad.lib"
call :revisa "deps\assimp\MSVC2022\lib\x64\Debug\assimp-vc143-mtd.lib"
call :revisa "deps\irrklang\lib\x64\irrKlang.lib"
echo.
echo DLL junto al ejecutable (src\bin):
call :revisa "bin\assimp-vc143-mtd.dll"
call :revisa "bin\irrKlang.dll"
call :revisa "bin\ikpMP3.dll"
echo.
echo Recursos del menu y del cubemap:
call :revisa "bin\shaders\menu_fondo.fs"
call :revisa "bin\textures\menu\titulo.png"
call :revisa "bin\textures\cubemap\submarino\posx.png"
echo.
if %FALTAN%==0 (
    echo Todo listo. Abre la solucion, elige Debug x64 y presiona F5.
) else (
    echo Faltan %FALTAN% archivo^(s^). Las .lib y .dll no estan en el repositorio:
    echo pideselas a Julian ^(su carpeta src\deps completa y las DLL de src\bin^).
)
echo.
pause
exit /b

:revisa
if exist "%~1" (
    echo   OK     %~1
) else (
    echo   FALTA  %~1
    set /a FALTAN+=1
)
exit /b
