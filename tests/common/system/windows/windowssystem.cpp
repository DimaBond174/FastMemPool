/*
 * This is the source code of SpecNet project
 * It is licensed under MIT License.
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#include "windowssystem.h"
#if defined(Windows)
#include <Windows.h>
#include <Windef.h>
#define PATH_MAX 260
#endif

//#include <unistd.h>
#include <limits.h>
//#include <linux/limits.h>
//#include <dlfcn.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
//#include <sys/socket.h>
//#include <sys/un.h>
#include <thread>
#include <algorithm>
#include <windows.h>
#include <stdio.h>
#include <ctime>

#define BUFFER_SIZE 256


std::string spec::getExePath()
{
    std::string _exePath;

    char buf[PATH_MAX];

    DWORD len = GetModuleFileNameA(GetModuleHandleA(0x0), buf, MAX_PATH);

    if (len > 0) {
            for (--len; len > 0; --len) {
                    if ('/' == buf[len] || '\\' == buf[len]) {
                            _exePath = std::string(buf, len);
                            break;
                    }
            }
    }

    return _exePath;
} // getExePath

std::string  spec::getExecName()
{
    std::string _exePath;

    char buf[PATH_MAX];

    DWORD len = GetModuleFileNameA(GetModuleHandleA(0x0), buf, MAX_PATH);

    if (len > 0) {
        _exePath = std::string(buf, len);
    }

    return _exePath;
} // getExecName


std::string  spec::execCmd(const char  *cmd)
{

    std::string result;
    system(cmd);
    //std::array<char, 128> buffer;
    //std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    //if (!pipe) throw std::runtime_error("popen() failed!");
    //while (!feof(pipe.get())) {
    //	if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
    //		result += buffer.data();
    //}
    //in deleter of shared_ptr: pclose(pipe);
    return result;
} // execCmd


static bool writeMailSlot(const std::string & slot, const std::string & msg) {
    HANDLE hFile = nullptr;
    bool re = false;
    //faux loop
    do {
        hFile = CreateFile(slot.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            (LPSECURITY_ATTRIBUTES)NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            (HANDLE)NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            //std::cerr << "FAIL writeMailSlot("
            //	<< slot
            //	<< ").CreateFile with "
            //	<< GetLastError() << std::endl;
            break;
        }

        BOOL fResult;
        DWORD cbWritten;

        fResult = WriteFile(hFile,
            msg.c_str(),
            msg.length(),
            &cbWritten,
            (LPOVERLAPPED)NULL);

        if (!fResult){
            std::cerr << "FAIL writeMailSlot("
                << slot
                << ").WriteFile with "
                << GetLastError() << std::endl;
            break;
        }
        re = true;
    } while (false);

    if (hFile) {
        CloseHandle(hFile);
    }
    return re;
}


static std::string getSockPathS(const char * serviceName) {
    std::string legal(serviceName);
    std::transform(legal.begin(), legal.end(), legal.begin(), [](char ch) {
        const char * legal = "abcdefghijklmnopqrstuvwxyz1234567890";
        for (const char *p = legal; *p; ++p) {
            if (*p == ch) { return ch; }
        }
        return 'a'; });
    std::string re("\\\\.\\mailslot\\");
    re.append(legal);
    return re;
}

std::string  spec::sendCmd(const char  *serviceName,  const char  *cmd)
{
    //1 Server created a mailslot
    //https://docs.microsoft.com/en-us/windows/desktop/ipc/creating-a-mailslot
    //2 This client will create a mailslot to receive a response.
    //https://docs.microsoft.com/en-us/windows/desktop/ipc/writing-to-a-mailslot
    //https://docs.microsoft.com/en-us/windows/desktop/ipc/reading-from-a-mailslot
    const std::string &srvMailSlot = getSockPathS(serviceName);
    std::string cliMailSlot(srvMailSlot);
    cliMailSlot.append(std::to_string(GetCurrentProcessId()));
    std::string re;
    HANDLE hSlot = nullptr;
    //faux loop
    do {
        //create answer slot
        HANDLE hSlot = CreateMailslot(cliMailSlot.c_str(),
            0,                             // no maximum message size
            MAILSLOT_WAIT_FOREVER,         // no time-out for operations
            (LPSECURITY_ATTRIBUTES)NULL); // default security

        if (hSlot == INVALID_HANDLE_VALUE) {
            std::cerr << "CreateMailslot("<<
                cliMailSlot
                <<") failed with error:"
                << GetLastError() << std::endl;
            break;
        }

        DWORD cbMessage, cMessage, cbRead;
        BOOL fResult;
        LPTSTR lpszBuffer;
        TCHAR achID[80];
        DWORD cAllMessages;
        HANDLE hEvent;
        OVERLAPPED ov;

        cbMessage = cMessage = cbRead = 0;
        std::string slotName("Slot");
        slotName.append(std::to_string((uint64_t)hSlot));

        hEvent = CreateEvent(NULL, FALSE, FALSE, slotName.c_str());
        if (NULL == hEvent) {
            std::cerr << "CreateEvent failed with error:"
                << GetLastError() << std::endl;
            break;
        }
        ov.Offset = 0;
        ov.OffsetHigh = 0;
        ov.hEvent = hEvent;

        cliMailSlot.push_back('$');
        cliMailSlot.append(cmd);
        std::time_t startTime = std::time(nullptr);
        if (writeMailSlot(srvMailSlot, cliMailSlot)) {
            while (re.empty() && std::time(nullptr) - startTime < 3) {
                fResult = GetMailslotInfo(hSlot, // mailslot handle
                    (LPDWORD)NULL,               // no maximum message size
                    &cbMessage,                   // size of next message
                    &cMessage,                    // number of messages
                    (LPDWORD)NULL);              // no read time-out

                if (!fResult) {
                    std::cerr << "GetMailslotInfo failed with error:"
                        << GetLastError() << std::endl;
                    break;
                }

                if (cbMessage == MAILSLOT_NO_MESSAGE) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,
                    lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage);
                if (NULL == lpszBuffer) {
                    std::cerr << "Oh no free RAM, GlobalAlloc==NULL"
                        << std::endl;
                    break;
                }
                lpszBuffer[0] = '\0';
                fResult = ReadFile(hSlot,
                    lpszBuffer,
                    cbMessage,
                    &cbRead,
                    &ov);

                if (!fResult) {
                    std::cerr << "ReadFile failed with "
                        << GetLastError() << std::endl;
                    GlobalFree((HGLOBAL)lpszBuffer);
                    break;
                }
                re = std::string(lpszBuffer);
                GlobalFree((HGLOBAL)lpszBuffer);
            }//while
        }
        CloseHandle(hEvent);
    } while (false);

    return re;
} // sendCmd


std::string  spec::getSockPath(const char  *serviceName)
{
  std::string legal (serviceName);
  std::transform(legal.begin(), legal.end(), legal.begin(),
    [](char ch)  {
      const char * legal = "abcdefghijklmnopqrstuvwxyz1234567890";
      for (const char *p = legal; *p; ++p) {
          if (*p==ch) { return ch; }
      }
      return 'a'; } );
  std::string re ("/var/tmp/");
  re.append(legal);
  return re;
} // getSockPath



//WindowsSystem::WindowsSystem()
//{

//}


//std::string WindowsSystem::getExePath() {
//	return getExePathS();
//}

//std::shared_ptr<ILib> WindowsSystem::openSharedLib(const char * libPath) {
//	return openSharedLibS(libPath);
//}

//std::shared_ptr<ILib> WindowsSystem::openSharedLibS(const char * libPath) {
//	std::shared_ptr<ILib> re;
//	//faux loop
//	do {
//		//https://msdn.microsoft.com/en-us/library/ms810279.aspx
//		//void * lib_handle = dlopen(libPath, RTLD_LAZY);
//		//HINSTANCE dllHandle = LoadLibrary(libPath);
//		//HINSTANCE dllHandle = LoadLibraryEx(libPath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
//		const std::string & test= getParentDir(libPath);
//		//https://stackoverflow.com/questions/24999520/setdlldirectory-loadlibrary-inside-a-dll
//		//https://docs.microsoft.com/en-us/windows/desktop/api/libloaderapi/nf-libloaderapi-loadlibraryexa
//		//https://docs.microsoft.com/ru-ru/windows/desktop/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories
//		if (!SetDllDirectory(
//				getParentDir(libPath).c_str())) {
//			break;
//		}
//		HINSTANCE dllHandle = LoadLibrary(libPath);

//		if (!dllHandle) {
//			std::cerr << "FAIL dlopenl: " << libPath << std::endl;
//			break;
//		}
//		// Reset errors
//		//dlerror();
//		std::shared_ptr<ILib> lib = std::make_shared<ILib>();

//		//https://docs.microsoft.com/en-us/windows/desktop/api/libloaderapi/nf-libloaderapi-getprocaddress
//		//from Developer Command Prompt
//		//DUMPBIN /ARCHIVEMEMBERS /CLRHEADER /DEPENDENTS /EXPORTS / IMPORTS / SUMMARY / SYMBOLS testssl.dll
//		//DUMPBIN /EXPORTS  testssl.dll
//		lib.get()->createInstance =
//			(TCreateFunc)GetProcAddress(dllHandle,
//				"createInstance");
					
//		if (!lib.get()->createInstance) {
//			std::cerr << "FAIL GetProcAddress( createInstance ): " << libPath << std::endl;
//			break;
//		}

//		lib.get()->deleteInstance =
//			(TDeleteFunc)GetProcAddress(dllHandle,
//				"deleteInstance");

//		if (!lib.get()->createInstance) {
//			std::cerr << "FAIL GetProcAddress( deleteInstance ): " << libPath << std::endl;
//			break;
//		}
	
//		lib.get()->lib_handle = dllHandle;
//		re = lib;
//	} while (false);
//	return re;
//}

//void WindowsSystem::closeSharedLib(const std::shared_ptr<ILib> &iLib) {
//	closeSharedLibS(iLib);
//}

//void WindowsSystem::closeSharedLibS(const std::shared_ptr<ILib> &iLib) {
//	if (iLib && iLib.get()->lib_handle) {
//		FreeLibrary((HINSTANCE)(iLib.get()->lib_handle));
//		iLib.get()->lib_handle = nullptr;
//	}
//}

//std::string WindowsSystem::getParentDir(const char * path) {
//	int lastPos = 0;
//	int i = 0;
//	for (; path[i]; ++i) {
//		if ('/' == path[i] || '\\' == path[i]) {
//			lastPos = i;
//		}
//	}
//	if (0 == lastPos) { lastPos = i - 1; }
//	if (lastPos > 0) { return std::string(path, lastPos); }
//	return std::string();
//}

//std::string WindowsSystem::getExePathS() {
//	std::string _exePath;

//	char buf[PATH_MAX];

//	DWORD len = GetModuleFileNameA(GetModuleHandleA(0x0), buf, MAX_PATH);

//	if (len > 0) {
//		for (--len; len > 0; --len) {
//			if ('/' == buf[len] || '\\' == buf[len]) {
//				_exePath = std::string(buf, len);
//				break;
//			}
//		}
//	}

//	return _exePath;
//}

//std::string WindowsSystem::getExeS() {
//	std::string _exePath;
//#if defined(Linux)
//	char buf[PATH_MAX];
//	ssize_t len = 0;
//	len = readlink("/proc/self/exe", buf, PATH_MAX);
//	if (len > 0) {
//		_exePath = std::string(buf, len);
//	}
//#endif
//	return _exePath;
//}

//std::string WindowsSystem::execCmd(const char * cmd) {
//	return execCmdS(cmd);
//}

//std::string WindowsSystem::execCmdS(const char * cmd) {
	
//	std::string result;
//	system(cmd);
//	//std::array<char, 128> buffer;
//	//std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
//	//if (!pipe) throw std::runtime_error("popen() failed!");
//	//while (!feof(pipe.get())) {
//	//	if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
//	//		result += buffer.data();
//	//}
//	//in deleter of shared_ptr: pclose(pipe);
//	return result;
//}

//std::string WindowsSystem::sendCmd(const char * serviceName, const char * cmd) {
//	return sendCmdS(serviceName, cmd);
//}


//bool WindowsSystem::writeMailSlot(const std::string & slot, const std::string & msg) {
//	HANDLE hFile = nullptr;
//	bool re = false;
//	//faux loop
//	do {
//		hFile = CreateFile(slot.c_str(),
//			GENERIC_WRITE,
//			FILE_SHARE_READ,
//			(LPSECURITY_ATTRIBUTES)NULL,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			(HANDLE)NULL);

//		if (hFile == INVALID_HANDLE_VALUE) {
//			//std::cerr << "FAIL writeMailSlot("
//			//	<< slot
//			//	<< ").CreateFile with "
//			//	<< GetLastError() << std::endl;
//			break;
//		}

//		BOOL fResult;
//		DWORD cbWritten;

//		fResult = WriteFile(hFile,
//			msg.c_str(),
//			msg.length(),
//			&cbWritten,
//			(LPOVERLAPPED)NULL);

//		if (!fResult){
//			std::cerr << "FAIL writeMailSlot("
//				<< slot
//				<< ").WriteFile with "
//				<< GetLastError() << std::endl;
//			break;
//		}
//		re = true;
//	} while (false);

//	if (hFile) {
//		CloseHandle(hFile);
//	}
//	return re;
//}

//std::string WindowsSystem::sendCmdS(const char * serviceName, const char * cmd) {
//	//1 Server created a mailslot
//	//https://docs.microsoft.com/en-us/windows/desktop/ipc/creating-a-mailslot
//	//2 This client will create a mailslot to receive a response.
//	//https://docs.microsoft.com/en-us/windows/desktop/ipc/writing-to-a-mailslot
//	//https://docs.microsoft.com/en-us/windows/desktop/ipc/reading-from-a-mailslot
//	const std::string &srvMailSlot = getSockPathS(serviceName);
//	std::string cliMailSlot(srvMailSlot);
//	cliMailSlot.append(to_string(GetCurrentProcessId()));
//	std::string re;
//	HANDLE hSlot = nullptr;
//	//faux loop
//	do {
//		//create answer slot
//		HANDLE hSlot = CreateMailslot(cliMailSlot.c_str(),
//			0,                             // no maximum message size
//			MAILSLOT_WAIT_FOREVER,         // no time-out for operations
//			(LPSECURITY_ATTRIBUTES)NULL); // default security

//		if (hSlot == INVALID_HANDLE_VALUE) {
//			std::cerr << "CreateMailslot("<<
//				cliMailSlot
//				<<") failed with error:"
//				<< GetLastError() << std::endl;
//			break;
//		}

//		DWORD cbMessage, cMessage, cbRead;
//		BOOL fResult;
//		LPTSTR lpszBuffer;
//		TCHAR achID[80];
//		DWORD cAllMessages;
//		HANDLE hEvent;
//		OVERLAPPED ov;

//		cbMessage = cMessage = cbRead = 0;
//		std::string slotName("Slot");
//		slotName.append(to_string(hSlot));

//		hEvent = CreateEvent(NULL, FALSE, FALSE, slotName.c_str());
//		if (NULL == hEvent) {
//			std::cerr << "CreateEvent failed with error:"
//				<< GetLastError() << std::endl;
//			break;
//		}
//		ov.Offset = 0;
//		ov.OffsetHigh = 0;
//		ov.hEvent = hEvent;
				
//		cliMailSlot.push_back('$');
//		cliMailSlot.append(cmd);
//		std::time_t startTime = std::time(nullptr);
//		if (writeMailSlot(srvMailSlot, cliMailSlot)) {
//			while (re.empty() && std::time(nullptr) - startTime < 3) {
//				fResult = GetMailslotInfo(hSlot, // mailslot handle
//					(LPDWORD)NULL,               // no maximum message size
//					&cbMessage,                   // size of next message
//					&cMessage,                    // number of messages
//					(LPDWORD)NULL);              // no read time-out

//				if (!fResult) {
//					std::cerr << "GetMailslotInfo failed with error:"
//						<< GetLastError() << std::endl;
//					break;
//				}

//				if (cbMessage == MAILSLOT_NO_MESSAGE) {
//					std::this_thread::sleep_for(std::chrono::milliseconds(100));
//					continue;
//				}

//				lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,
//					lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage);
//				if (NULL == lpszBuffer) {
//					std::cerr << "Oh no free RAM, GlobalAlloc==NULL"
//						<< std::endl;
//					break;
//				}
//				lpszBuffer[0] = '\0';
//				fResult = ReadFile(hSlot,
//					lpszBuffer,
//					cbMessage,
//					&cbRead,
//					&ov);

//				if (!fResult) {
//					std::cerr << "ReadFile failed with "
//						<< GetLastError() << std::endl;
//					GlobalFree((HGLOBAL)lpszBuffer);
//					break;
//				}
//				re = std::string(lpszBuffer);
//				GlobalFree((HGLOBAL)lpszBuffer);
//			}//while
//		}
//		CloseHandle(hEvent);
//	} while (false);

//	return re;
//}

//std::string WindowsSystem::getSockPath(const char * serviceName) {
//	return getSockPathS(serviceName);
//}

//std::string WindowsSystem::getSockPathS(const char * serviceName) {
//	std::string legal(serviceName);
//	std::transform(legal.begin(), legal.end(), legal.begin(), [](char ch) {
//		const char * legal = "abcdefghijklmnopqrstuvwxyz1234567890";
//		for (const char *p = legal; *p; ++p) {
//			if (*p == ch) { return ch; }
//		}
//		return 'a'; });
//	std::string re("\\\\.\\mailslot\\");
//	re.append(legal);
//	return re;
//}

//bool WindowsSystem::waitForSUCCESS(TWaitFunc f, void * ptr,
//	int msRepeat,
//	int msTimeout) {
//	auto start = std::chrono::system_clock::now();
//	while (std::chrono::system_clock::now() - start < std::chrono::milliseconds(msTimeout)) {
//		if ((*f)(ptr)) {
//			return true;
//		}
//		std::this_thread::sleep_for(std::chrono::milliseconds(msRepeat));
//	}//while
//	return false;
//}
