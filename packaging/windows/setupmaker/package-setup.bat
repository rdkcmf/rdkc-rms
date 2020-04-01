@git rev-list --count HEAD >commit.tmp
@set /p GIT_COMMIT=<commit.tmp
del README.txt
del rdkcms-1.7.1.%GIT_COMMIT%-x86_64-Windows_7.zip
copy holding\README.txt .
..\zip.exe rdkcms-1.7.1.%GIT_COMMIT%-x86_64-Windows_7.zip setup.exe README.txt
