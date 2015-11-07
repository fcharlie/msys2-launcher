/**
* MSYS2 Launcher
*
**/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <Windows.h>
#include <Shlwapi.h>
#define  SECURITY_WIN32
#include <Security.h>
#include <string>
#if defined(_MSC_VER)
#pragma comment(lib,"user32")
#pragma comment(lib,"kernel32")
#pragma comment(lib,"gdi32")
#pragma comment(lib,"Shlwapi")
#if defined(_WIN32_WINNT) &&_WIN32_WINNT>=_WIN32_WINNT_WIN8
#include <Processthreadsapi.h>
#endif
#endif
#include "cpptoml.h"

struct LauncherStructure{
  std::wstring env;
  std::wstring root;
  std::wstring wd;
  std::wstring mintty;
  std::wstring icon;
  std::wstring shell;
  std::wstring shellArgs;
  bool enableIcon;
  bool enableZshell;
};
class Characters{
private:
    char *str;
public:
    Characters(const wchar_t *wstr):str(nullptr)
    {
      if(wstr==nullptr) return ;
      int iTextLen=WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
      str = new char[iTextLen + 1];
      if(str==nullptr) return;
      str[iTextLen]=0;
      WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, iTextLen,NULL, NULL);
    }
    const char *Get()
    {
        if(!str)
            return nullptr;
        return const_cast<const char *>(str);
    }
    ~Characters()
    {
        if(str)
            delete[] str;
    }
};



class WCharacters{
private:
    wchar_t *wstr;
public:
    WCharacters(const char *str):wstr(nullptr)
    {
        if(str==nullptr)
            return ;
        int unicodeLen = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        if(unicodeLen==0)
            return ;
        wstr = new wchar_t[unicodeLen + 1];
        if(wstr==nullptr) return;
        wstr[unicodeLen]=0;
        ::MultiByteToWideChar(CP_UTF8, 0,str, -1, (LPWSTR)wstr,unicodeLen);
    }
    const wchar_t *Get()
    {
        if(!wstr)
            return nullptr;
        return const_cast<const wchar_t *>(wstr);
    }
    ~WCharacters()
    {
        if(wstr)
            delete[] wstr;
    }
};

bool GetProcessImageFileFolder(std::wstring &wstr)
{
  wchar_t szbuf[32767];
  if(!GetModuleFileNameW(nullptr,szbuf,32767))
    return false;
  PathRemoveFileSpecW(szbuf);
  wstr=szbuf;
  return true;
}

bool LauncherProfile(const wchar_t *cf,LauncherStructure &config)
{
  std::wstring pfile;
  if(cf==nullptr){
    std::wstring folder;
    if(!GetProcessImageFileFolder(folder)){
      MessageBoxW(nullptr,
        L"GetModuleFileNameW Failed!",
        L"Internal System Error",
        MB_OK|MB_ICONERROR);
      return false;
    }
    pfile=folder+L"/launcher.toml";
    if(_waccess_s(pfile.c_str(),04)!=0){
      pfile=folder+L"/launcher.exe.toml";
      if(_waccess_s(pfile.c_str(),04)!=0){
        MessageBoxW(nullptr,
          folder.c_str(),
          L"Cannot open launcher.toml or launcher.exe.toml on this path",
          MB_OK|MB_ICONERROR);
        return false;
      }
    }
  }else{
    pfile=cf;
  }
  Characters chars(pfile.c_str());
  std::shared_ptr<cpptoml::table> g;
  try {
    g = cpptoml::parse_file(chars.Get());
  } catch (const cpptoml::parse_exception &e) {
    (void)e;
    MessageBoxW(nullptr,
      pfile.c_str(),
      L"Cannot parser toml-format profile!",
      MB_OK|MB_ICONERROR);
    return false;
  }
  auto keyBind = [&](const char *key, const wchar_t *v, std::wstring &sv) {
    if (g->contains_qualified(key)) {
      std::string astr = g->get_qualified(key)->as<std::string>()->get();
      printf("Key Value: %s: %s\n",key,astr.c_str());
      WCharacters was(astr.c_str());
      sv=was.Get();
      return;
    }
    sv = v;
  };
  auto keyBool = [&](const char *key, bool b, bool &cb) {
    if (g->contains_qualified(key)) {
      cb = g->get_qualified(key)->as<bool>()->get();
      // printf("Key and value: %s:%s\n",key,cb?"true":"false");
      return;
    }
    cb = b;
  };
  keyBind("Launcher.MSYSTEM",L"MINGW64",config.env);
  keyBind("Launcher.Root",L"C:/MSYS2",config.root);
  keyBind("Launcher.WD",L"C:/MSYS2/usr/bin",config.wd);
  keyBind("Launcher.Mintty",L"/usr/bin/mintty.exe",config.mintty);
  keyBind("Launcher.ICONPath",L"/msys2.ico",config.icon);
  keyBind("Launcher.Shell",L"bash",config.shell);
  keyBind("Launcher.ShellArgs",L"",config.shellArgs);
  keyBool("Launcher.EnableICON",true,config.enableIcon);
  keyBool("Launcher.EnableZshell",false,config.enableZshell);
  return true;
}

bool PutEnvironmentVariableW(const wchar_t *name,const wchar_t *va)
{
  wchar_t buffer[32767];
  auto dwSize=GetEnvironmentVariableW(name,buffer,32767);
  if(dwSize<=0){
    return SetEnvironmentVariableW(name,va)?true:false;
  }
  if(dwSize>=32766) return false;
  if(buffer[dwSize-1]!=';'){
    buffer[dwSize]=';';
    dwSize++;
    buffer[dwSize]=0;
  }
  wcscat_s(buffer,32767-dwSize,va);
  return SetEnvironmentVariableW(name,buffer)?true:false;
}

/**
SCS_32BIT_BINARY
0
A 32-bit Windows-based application
----
SCS_64BIT_BINARY
6
A 64-bit Windows-based application.
-----
SCS_DOS_BINARY
1
An MS-DOS – based application
----
SCS_OS216_BINARY
5
A 16-bit OS/2-based application
----
SCS_PIF_BINARY
3
A PIF file that executes an MS-DOS – based application
----
SCS_POSIX_BINARY
4
A POSIX – based application
----
SCS_WOW_BINARY
2
A 16-bit Windows-based application
**/


// struct TargetArchitecture{
//   int typeId;
//   const wchar_t *name;
// };

bool ProcessTargetIsMatch(const wchar_t *binary)
{
  // DWORD dwSelf,dwBinary;
  // TargetArchitecture targetList[]={
  //   {SCS_32BIT_BINARY,L"32-bit Windows-based application"}, //0
  //   {SCS_64BIT_BINARY,L"64-bit Windows-based application"}, //6
  //   {SCS_DOS_BINARY,L"MS-DOS – based application"}, //1
  //   {SCS_OS216_BINARY,L"16-bit OS/2-based application"},//5
  //   {SCS_PIF_BINARY,L"PIF file that executes an MS-DOS – based application"},//3
  //   {SCS_POSIX_BINARY,L"POSIX – based application"},//4
  //   {SCS_WOW_BINARY,L"16-bit Windows-based application"} //2
  // };
  const wchar_t *TargetList[]={
    L"32-bit Windows-based application",
    L"MS-DOS – based application",
    L"16-bit Windows-based application",
    L"PIF file that executes an MS-DOS – based application",
    L"POSIX – based application",
    L"16-bit OS/2-based application",
    L"64-bit Windows-based application",
    L"Unknown Target application"
  };
  DWORD dwBinary,dwSelf;
  wchar_t selfPath[32767];
  if(!GetModuleFileNameW(nullptr,selfPath,32767))
    return false;
  if(GetBinaryTypeW(binary,&dwBinary)==0)
    return false;
  if(GetBinaryTypeW(selfPath,&dwSelf)==0)
    return false;
  if(dwSelf==dwBinary) return true;
  std::wstring message=L"Not match target Architecture \nLauncher self is ";
  message+=TargetList[dwSelf>7?7:dwSelf];
  message+=L"\nBut Mintty ("+std::wstring(binary)+std::wstring(L" ) is ");
  message+=TargetList[dwBinary>7?7:dwBinary];
  MessageBoxW(nullptr,message.c_str(),L"Process Architecture not match ",MB_OK|MB_ICONERROR);
  return false;
}


bool StartupMiniPosixEnv(LauncherStructure &config)
{
  SetCurrentDirectoryW(config.root.c_str());
  SetEnvironmentVariableW(L"MSYSTEM",config.env.c_str());
  SetEnvironmentVariableW(L"WD",config.wd.c_str());
  SetEnvironmentVariableW(L"MSYSCON",L"mintty.exe");
  //PutEnvironmentVariableW(L"PATH",config.wd.c_str());
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOW;
  std::wstring binary=config.root+config.mintty;
  if(!ProcessTargetIsMatch(binary.c_str()))
  {
    //MessageBoxW(nullptr,binary.c_str(),L"Launcher Taget match not with:",MB_OK);
    return false;
  }
  std::wstring args=std::wstring(L"-i")+config.icon+std::wstring(L" /usr/bin/");
  /**
  std::wstring args=std::wstring(L"-o RelaunchCommand=\"")+config.root;
  if(config.enableZshell){
      args+=std::wstring(L"\" -o RelauncheDisplayName=\"Mini Posix Environment\"  -i ");
  }else{
    args+=config.shell+std::wstring(L"\" -o RelauncheDisplayName=\"Mini Posix Environment\"  -i ");
  }
  **/
  if(config.enableZshell){
    args+=std::wstring(L"zsh --login -i ");
  }else{
    args+=config.shell+std::wstring(L" --login -i ");
  }
  if(!config.shellArgs.empty())
    args.append(config.shellArgs);
  DWORD maxLength=MAX_PATH;
  wchar_t name[MAX_PATH]={0};
  if(GetUserNameW(name, &maxLength)==0){
    DWORD dw= GetLastError();
    wchar_t sw[32]={0};
    wsprintfW(sw,L"GetLastError :%d\n",dw);
    MessageBoxW(nullptr,sw,L"Cannot Invoker GetUserNameW",MB_OK);
    return false;
  }
  //MessageBoxW(nullptr,home.c_str(),L"Args",MB_OK);
  wchar_t zargs[32767]={0};
  args.copy(zargs,args.size(),0);
  //MessageBoxW(nullptr,zargs,L"Args",MB_OK);
  BOOL result = CreateProcessW(
    binary.c_str(),
    zargs,
    NULL,
    NULL,
    NULL,
    CREATE_NEW_CONSOLE,
    NULL,
    NULL,
    &si,
    &pi
    );
  if(result){
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  //ShellExecuteW(nullptr,L"open",binary.c_str(),args.c_str(),home.c_str(),0);
  return true;
}

#ifdef __GNUC__
int WINAPI WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow
  ){
  WCharacters wstr(const_cast<const char*>(lpCmdLine));
  auto c=wcsdup(wstr.Get());
  auto l=wWinMain(hInstance,hPrevInstance,c,nCmdShow);
  free(c);
  return l;
}
#endif

int WINAPI wWinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPWSTR lpCmdLine,
  int nCmdShow)
{
  LauncherStructure config;
  const wchar_t *cf=nullptr;
  int Argc;
  LPWSTR *Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
  if(Argc>=3){
    if(wcscmp(Argv[1],L"-c")&&_waccess_s(Argv[2],04)==0)
      cf=Argv[1];
  }
  if(!LauncherProfile(cf,config)){
    LocalFree(Argv);
    return 1;
  }
  LocalFree(Argv);
  if(!StartupMiniPosixEnv(config)){
    //MessageBoxW(nullptr,L"Please check your launcher setting !",L"Cannot Start MSYS2",MB_OK|MB_ICONERROR);
    return 2;
  }
  return 0;
}