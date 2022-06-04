all: so-cpp

so-cpp: so-cpp.c
	cl so-cpp.c

clean:
	del /Q /F *.obj so-cpp.exe
