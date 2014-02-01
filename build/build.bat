@echo off

set SDK_INC_DIR=C:\Users\Silas\Downloads\2013\scs_sdk_1_0\include
set INSTALL_DIR=C:\Program Files (x86)\Steam\steamapps\common\Euro Truck Simulator 2\bin\win_x86\plugins
set SRC_DIR=..\source\ets2_plugin

set SRC_FILES=%SRC_DIR%\plugin.def %SRC_DIR%\plugin.cpp %SRC_DIR%\serial.cpp %SRC_DIR%\options.cpp

set OPTIONS=/nologo /W4 /LD /MD /EHsc /O2

@echo on

cl %OPTIONS% /I%SDK_INC_DIR% %SRC_FILES% /link /OUT:dash_plugin.dll

xcopy dash_plugin.dll "%INSTALL_DIR%" /D

