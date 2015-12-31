#GNU Makefile

CXX?=g++
LD=g++
RC=windres
CFLAGS= -DNODEBUG -DUNICODE -D_UNICODE -O2 
CXXFLAGS=-std=c++11 -g -Wall  -Wunused-variable 
LDFALGS=-static -static-libgcc  -static-libstdc++
LINKFLAGS=-lkernel32 -ladvapi32 -lshell32 -lgdi32 -luser32 -lcomctl32 -lshlwapi -lsecur32



all:launcher_mingw.o launcher.o
	$(LD)  $(LDFALGS) launcher.o launcher_mingw.o -o launcher.exe $(LINKFLAGS)

	
launcher_mingw.o:launcher_mingw.rc
	$(RC) launcher_mingw.rc launcher_mingw.o
	
launcher.o:
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) launcher.cpp
	

clean:
	-rm  *.o  *.exe