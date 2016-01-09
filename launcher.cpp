/**
* MSYS2 Launcher
* Copyright (C) 2016, ForceStudio All Rights Reserved.
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <iostream>
#include <string>
#include <vector>

#ifndef __CYGWIN__
#define  SECURITY_WIN32
#include <Security.h>
#include <StrSafe.h>
#else
#define STRSAFE_MAX_CCH 2147483647
#define STRSAFE_E_INVALID_PARAMETER ((HRESULT)0x80070057L)

inline HRESULT StringCchLengthW(
    LPCWSTR psz,
    size_t  cchMax,
    size_t  *pcchLength
)
{
  HRESULT hr = S_OK;
  size_t cchMaxPrev = cchMax;
  while(cchMax && (*psz!=L'\0')) {
    psz++;
    cchMax--;
  }
  if(cchMax==0) hr = STRSAFE_E_INVALID_PARAMETER;
  if(pcchLength) {
    if(SUCCEEDED(hr)) *pcchLength = cchMaxPrev - cchMax;
    else *pcchLength = 0;
  }
  return hr;
}

HRESULT StringCchCopyW(
    LPWSTR  pszDest,
    size_t  cchDest,
    LPCWSTR pszSrc
){
    HRESULT hr=S_OK;
    if(cchDest==0||pszDest==nullptr) return STRSAFE_E_INVALID_PARAMETER;
    while(cchDest &&(*pszSrc!=L'\0')){
        *pszDest++=*pszSrc++;
        cchDest--;
    }
    if(cchDest==0){
        pszDest--;
        hr=STRSAFE_E_INVALID_PARAMETER;
    }
    *pszDest=L'\0';
    return hr;
}

inline HRESULT StringCchCatW(
    LPWSTR  pszDest,
    size_t  cchDest,
    LPCWSTR pszSrc){
    HRESULT hr = S_OK;
    if(cchDest==0||pszDest==nullptr) return STRSAFE_E_INVALID_PARAMETER;
    size_t lengthDest;
    if(StringCchLengthW(pszDest,cchDest,&lengthDest)!=S_OK){
        hr=STRSAFE_E_INVALID_PARAMETER;
    }else{
        hr=StringCchCopyW(pszDest+lengthDest,cchDest-lengthDest,pszSrc);
    }
    return hr;
}

#endif



#if defined(_MSC_VER)
#if defined(_WIN32_WINNT) &&_WIN32_WINNT>=_WIN32_WINNT_WIN8
#include <Processthreadsapi.h>
#endif
#endif

#ifdef __GNUC__
#define launcherStartup WinMain
#define CHARPTR LPSTR
#elif defined(_MSC_VER)
#define launcherStartup wWinMain
#define CHARPTR LPWSTR
#else
#error "Cannot support this compiler !"
#endif

#include "cpptoml.h"

struct LauncherStructure{
  std::wstring env;
  std::wstring root;
  std::wstring wd;
  std::wstring mintty;
  std::wstring icon;
  std::wstring otherShell;
  std::wstring shellArgs;
  std::vector<std::wstring> appendPath;
  bool enableZshell;
  bool clearEnvironment;
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
    if(!PathFileExistsW(pfile.c_str())){
      pfile=folder+L"/launcher.exe.toml";
      if(!PathFileExistsW(pfile.c_str())){
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
    std::cout << (*g) << std::endl;
  } catch (const cpptoml::parse_exception &e) {
    (void)e;
    MessageBoxW(nullptr,
      pfile.c_str(),
      L"Cannot parser toml-format profile!",
      MB_OK|MB_ICONERROR);
    return false;
  }
  auto Strings = [&](const char *key, const wchar_t *v, std::wstring &sv) {
    if (g->contains_qualified(key)) {
      std::string astr = g->get_qualified(key)->as<std::string>()->get();
      WCharacters was(astr.c_str());
      sv=was.Get();
      return;
    }
    if(v){
        sv = v;
      }
  };
  auto Boolean = [&](const char *key, bool b) {
    if (g->contains_qualified(key)) {
      return g->get_qualified(key)->as<bool>()->get();
    }
    return b;
  };
  auto Vector = [&](const char *key, std::vector<std::wstring> &v) {
    if (g->contains_qualified(key) && g->get_qualified(key)->is_array()) {
      auto av = g->get_qualified(key)->as_array();
      for (auto &e : av->get()) {
        WCharacters was(e->as<std::string>()->get().c_str());
        v.push_back(was.Get());
      }
    }
  };
  Strings("Launcher.MSYSTEM",L"MINGW64",config.env);
  Strings("Launcher.Root",L"C:/MSYS2",config.root);
  Strings("Launcher.WD",L"C:/MSYS2/usr/bin",config.wd);
  Strings("Launcher.Mintty",L"C:/MSYS2/usr/bin/mintty.exe",config.mintty);
  Strings("Launcher.ICONPath",L"/msys2.ico",config.icon);
  Strings("Launcher.Shell",L"bash",config.otherShell);
  Strings("Launcher.AppendShellArgs",nullptr,config.shellArgs);
  Vector("Launcher.AppendPath",config.appendPath);
  config.enableZshell=Boolean("Launcher.EnableZshell",false);
  config.clearEnvironment=Boolean("Launcher.UseClearEnv",false);
  return true;
}

enum KEnvStateMachine : int {
  kClearReset = 0,
  kEscapeAllow = 1,
  kMarkAllow = 2,
  kBlockBegin = 3,
  kBlockEnd = 4
};

static bool ResolveEnvironment(const std::wstring &k, std::wstring &v) {
  wchar_t buffer[32767];
  auto dwSize=GetEnvironmentVariableW(k.c_str(),buffer,32767);
  if(dwSize==0||dwSize>=32767)
    return false;
  buffer[dwSize]=0;
  v=buffer;
  return true;
}

static bool EnvironmentExpend(std::wstring &va){
  if (va.empty())
    return false;
  std::wstring ns, ks;
  auto p = va.c_str();
  auto n = va.size();
  int pre = 0;
  size_t i = 0;
  KEnvStateMachine state = kClearReset;
  for (; i < n; i++) {
    switch (p[i]) {
    case '`': {
      switch (state) {
      case kClearReset:
        state = kEscapeAllow;
        break;
      case kEscapeAllow:
        ns.push_back('`');
        state = kClearReset;
        break;
      case kMarkAllow:
        state = kEscapeAllow;
        ns.push_back('$');
        break;
      case kBlockBegin:
        continue;
      default:
        ns.push_back('`');
        continue;
      }
    } break;
    case '$': {
      switch (state) {
      case kClearReset:
        state = kMarkAllow;
        break;
      case kEscapeAllow:
        ns.push_back('$');
        state = kClearReset;
        break;
      case kMarkAllow:
        ns.push_back('$');
        state = kClearReset;
        break;
      case kBlockBegin:
      case kBlockEnd:
      default:
        ns.push_back('$');
        continue;
      }
    } break;
    case '{': {
      switch (state) {
      case kClearReset:
      case kEscapeAllow:
        ns.push_back('{');
        state = kClearReset;
        break;
      case kMarkAllow: {
        state = kBlockBegin;
        pre = i;
      } break;
      case kBlockBegin:
        ns.push_back('{');
        break;
      default:
        continue;
      }
    } break;
    case '}': {
      switch (state) {
      case kClearReset:
      case kEscapeAllow:
        ns.push_back('}');
        state = kClearReset;
        break;
      case kMarkAllow:
        state = kClearReset;
        ns.push_back('$');
        ns.push_back('}');
        break;
      case kBlockBegin: {
        ks.assign(&p[pre + 1], i - pre - 1);
        std::wstring v;
        if (ResolveEnvironment(ks, v))
          ns.append(v);
        state = kClearReset;
      } break;
      default:
        continue;
      }
    } break;
    default: {
      switch (state) {
      case kClearReset:
        ns.push_back(p[i]);
        break;
      case kEscapeAllow:
        ns.push_back('`');
        ns.push_back(p[i]);
        state = kClearReset;
        break;
      case kMarkAllow:
        ns.push_back('$');
        ns.push_back(p[i]);
        state = kClearReset;
        break;
      case kBlockBegin:
      default:
        continue;
      }
    } break;
    }
  }
  va = ns;
  return true;
}

bool ClearEnvironmentVariableW(){
  //WINDIR expend like C:\WINDOW
  std::wstring path=L"${WINDIR}\\System32;${WINDIR};${WINDIR}\\System32\\Wbem;${WINDIR}\\System32\\WindowsPowerShell\\v1.0\\";
  if(EnvironmentExpend(path)){
    SetEnvironmentVariableW(L"PATH",path.c_str());
    return true;
  }
  return false;
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
  StringCchCatW(buffer,32767-dwSize,va);
  return SetEnvironmentVariableW(name,buffer)?true:false;
}

bool PutPathEnvironmentVariableW(std::vector<std::wstring> &pv){
  std::wstring path(L";");
  for(auto &s:pv){
    path.append(s);
    path.push_back(';');
  }
  return PutEnvironmentVariableW(L"PATH",path.c_str());
}

bool StartupMiniPosixEnv(LauncherStructure &config)
{
  SetCurrentDirectoryW(config.root.c_str());
  if(config.clearEnvironment){
    ClearEnvironmentVariableW();
  }
  SetEnvironmentVariableW(L"MSYSTEM",config.env.c_str());
  SetEnvironmentVariableW(L"WD",config.wd.c_str());
  SetEnvironmentVariableW(L"MSYSCON",L"mintty.exe");

  if(!config.appendPath.empty()){
    PutPathEnvironmentVariableW(config.appendPath);
  }

  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOW;

  int const ArraySize=8192;
  wchar_t cmdline[ArraySize]={0};
  auto hr=StringCchCatW(cmdline,ArraySize,config.mintty.c_str());
  StringCchCatW(cmdline,ArraySize,L" -i ");
  StringCchCatW(cmdline,ArraySize,config.icon.c_str());
  StringCchCatW(cmdline,ArraySize,L" /usr/bin/");
  if(config.enableZshell){
      StringCchCatW(cmdline,ArraySize,L"zsh --login ");
  }else{
      StringCchCatW(cmdline,ArraySize,config.otherShell.c_str());
      StringCchCatW(cmdline,ArraySize,L" --login ");
  }
//   auto hr=StringCchPrintfW(cmdline,ArraySize,L"%s -i%s /usr/bin/%s --login ",
//   config.mintty.c_str(),
//   config.icon.c_str(),
//   config.enableZshell?L"zsh":config.otherShell.c_str());
  if(hr!=S_OK){
      MessageBoxW(nullptr,L"Please check your profile !",L"Parse commandline failed !",MB_OK|MB_ICONERROR);
      return false;
  }
  if(!config.shellArgs.empty()){
      StringCchCatW(cmdline,ArraySize,config.shellArgs.c_str());  
  }
  ///CreateProcess
  BOOL result = CreateProcessW(
    NULL,
    cmdline,
    NULL,
    NULL,
    TRUE,
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
  return true;
}

int WINAPI launcherStartup(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  CHARPTR lpCmdLine,
  int nCmdShow)
{
  LauncherStructure config;
  const wchar_t *cf=nullptr;
  int Argc;
  LPWSTR *Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
  if(Argc>=3){
    if(wcscmp(Argv[1],L"-c")&&PathFileExistsW(Argv[2]))
      cf=Argv[1];
  }
  if(!LauncherProfile(cf,config)){
    LocalFree(Argv);
    return 1;
  }
  LocalFree(Argv);
  return StartupMiniPosixEnv(config)?2:0;
}
