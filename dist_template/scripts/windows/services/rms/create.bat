@SETLOCAL
@set CURRDIR=%~dp0..\..\
@sc delete RDKCMediaServer
@sc create RDKCMediaServer binPath= "%CURRDIR%services\srvany.exe" type= own start= auto DisplayName= "RDKC Media Server"
@if errorlevel==1 goto ERROR
@echo Windows Registry Editor Version 5.00 > reg_mod.reg
@echo [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\RDKCMediaServer\Parameters] >> reg_mod.reg
@SET BTMP="Application"="%CURRDIR%rdkcms.exe"
@SET BTMP=%BTMP:\=\\%
@echo %BTMP% >> reg_mod.reg
@echo "AppParameters"="config.lua" >> reg_mod.reg
@SET BTMP="AppDirectory"="%CURRDIR%config" >> reg_mod.reg
@SET BTMP=%BTMP:\=\\%
@echo %BTMP% >> reg_mod.reg
@call "reg_mod.reg"
@if errorlevel==1 goto ERROR
@goto END
:ERROR
@choice /C y /T 10 /D y /M "This script needs to be run with administrative privileges. Press y to exit"
:END
@ENDLOCAL 
@exit
