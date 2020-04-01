@sc stop RDKCMediaServer
@sc delete RDKCMediaServer
@if errorlevel==1 goto ERROR
@goto END
:ERROR
@choice /C y /T 10 /D y /M "This script needs to be run with administrative privileges. Press y to exit"
:END
@exit