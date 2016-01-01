###MSYS2 Launcher
This helper tools.

GCC build     
>make 

MSVC build   
>nmake -f msvc.mak

launcher.toml setting：   
```toml
#MSYS Launcher configure
[Launcher]
# MSYSTEM hava 3 type MSYS MINGW32 MINGW64
MSYSTEM="MINGW64"
Root="D:/MPOSIX2"
WD="D:/MPOSIX2/usr/bin"
Mintty="D:/MPOSIX2/usr/bin/mintty.exe"
EnableZshell=false
Shell="bash"
UseClearEnv=true
ICONPath="/msys2.ico"
#AppendShellArgs="Hello world"
#AppendPath=[
#  "D:/Tools/Path"
#]
```

UseClearEnv will clear PATH
```
${WINDIR}\\System32;${WINDIR};${WINDIR}\\System32\\Wbem;${WINDIR}\\System32\\WindowsPowerShell\\v1.0\\
```

AppendPath can set, add path to env


##Other
Dont use clang compiled, because clang link static failed.