@rem compile C source file with given name into NES file
@rem useful to compile few projects at once without repeating the build script

@set CC65_HOME=..\

@if "%PATH%"=="%PATH:cc65=%" @PATH=%PATH%;%CC65_HOME%bin\

@ca65 crt0.s || goto fail

@cc65 -Oi %1.c --add-source || goto fail

@ca65 %1.s || goto fail

@ld65 -C %2.cfg -o %1.nes crt0.o %1.o runtime.lib || goto fail

goto exit

:fail

pause

:exit

@del %1.s