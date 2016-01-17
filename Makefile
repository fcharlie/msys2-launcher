#GNU Makefile

CXX=g++
LD=g++
RC=windres
CFLAGS= -DNODEBUG -DUNICODE -D_UNICODE -O2 
CXXFLAGS=-std=c++11 -g -Wall  -Wunused-variable 
LDFALGS= -mwindows -static -static-libgcc  -static-libstdc++
LINKFLAGS=-lkernel32 -ladvapi32 -lshell32 -lgdi32 -luser32 -lcomctl32 -lshlwapi -lsecur32
TARGET=launcher.exe


all:$(TARGET)

$(TARGET):launcher.o launcher.res
	$(LD)  $(LDFALGS) launcher.o launcher.res -o launcher.exe $(LINKFLAGS)

	
launcher.res:launcher.rc
	$(RC) -c 65001 $< -O coff -o $@
	
launcher.o:launcher.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) launcher.cpp
	
install:$(TARGET)
	strip $(TARGET)

clean:
	-rm  *.o  *.res *.exe