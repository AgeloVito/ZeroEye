#include <iostream>
#include <windows.h>
#include <vector>
#include <io.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include "Sign.hpp"
#include "IM_EX_Ports.hpp"


bool isSign = false;
void EchoFunc(std::string dllFile,std::string txtFile,bool flag) {

    std::ofstream DllFuncFile(txtFile);
    std::vector<std::string> Funclist;
    ListExportedFunctions(dllFile.c_str(), true, Funclist);
    for (const auto& Func : Funclist) {
        DllFuncFile << "\textern \"C\" __declspec(dllexport) int " << Func << "() {     MessageBoxA(0,__FUNCTION__,0,0);    return 0;   }" << std::endl;
        if (flag)
        {
            std::cout << Func << std::endl;
        }
    }
    DllFuncFile.close();
}
void RenameDirectory(const std::filesystem::path& targetDir, const std::string& newDirName) {
    std::filesystem::path newDirPath = targetDir.parent_path() / newDirName;

    // 重命名目录
    try {
        if (std::filesystem::exists(targetDir)) {
            std::filesystem::rename(targetDir, newDirPath);
        }
        else {
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        
    }
}
bool Is_SystemDLL(const char* dllName) {

    HMODULE hModule = LoadLibraryExA(dllName, NULL, DONT_RESOLVE_DLL_REFERENCES);

    if (hModule)
    {
        FreeLibrary(hModule);
        return true;
    }
    else {
        char searchPath[MAX_PATH];
        DWORD SearchID = SearchPathA(NULL, dllName, ".dll", MAX_PATH, searchPath, NULL);
        if (SearchID > 0)
        {
            return true;
        }
    }

    return false;

}

void ViewImportedDLLs(const char* filePath, std::vector<std::string>& DllList ,bool & is64Bit) {
    // 设置错误模式，防止弹出错误信息框
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        //std::cerr << "Failed to open file." << std::endl;
        return;
    }

    HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
    if (!hMapping) {
        CloseHandle(hFile);
        //std::cerr << "Failed to create file mapping." << std::endl;
        return;
    }

    LPVOID pMappedFile = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!pMappedFile) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        //std::cerr << "Failed to map view of file." << std::endl;
        return;
    }

    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(pMappedFile);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        UnmapViewOfFile(pMappedFile);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        //std::cerr << "Invalid DOS header signature." << std::endl;
        return;
    }

    // 获取 NT Headers 的地址
    PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(pMappedFile) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        UnmapViewOfFile(pMappedFile);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        //std::cerr << "Invalid NT header signature." << std::endl;
        return;
    }

    is64Bit = (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);

    DWORD importTableRVA;

    if (is64Bit) {
        PIMAGE_NT_HEADERS64 ntHeaders64 = reinterpret_cast<PIMAGE_NT_HEADERS64>(ntHeaders);
        importTableRVA = ntHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    }
    else {
        PIMAGE_NT_HEADERS32 ntHeaders32 = reinterpret_cast<PIMAGE_NT_HEADERS32>(ntHeaders);
        importTableRVA = ntHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    }
    if (importTableRVA == 0) {
        //std::cerr << "No export table found." << std::endl;
        UnmapViewOfFile(pMappedFile);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    PIMAGE_IMPORT_DESCRIPTOR importDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<BYTE*>(pMappedFile) + importTableRVA);
    while (importDescriptor->Name != NULL) {
        // 检查 importDescriptor 是否为有效指针
        if (IsBadReadPtr(importDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR))) {
            break;
        }

        // 计算 DLL 名称的地址
        char* dllName = reinterpret_cast<char*>(reinterpret_cast<BYTE*>(pMappedFile) + importDescriptor->Name);

        // 检查 dllName 的有效性
        if (dllName && !IsBadStringPtrA(dllName, MAX_PATH) && strlen(dllName) > 0) {
            DllList.push_back(dllName);  // 只有在 dllName 有效时才加入列表
        }

        importDescriptor++;

        // 终止条件，避免越界
        if (importDescriptor->Characteristics == NULL) {
            break;
        }
    }
    UnmapViewOfFile(pMappedFile);
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return;


}

// 递归检查 DLL 的依赖项，处理所有层级的依赖链
void Recursive_CheckDLL(const std::string& basePath, const std::string& dllName, int depth, std::unordered_set<std::string>& checkedDlls) {
    // 检查是否已经扫描过该 DLL，避免重复扫描
    if (checkedDlls.count(dllName)) return;
    checkedDlls.insert(dllName);

    // 判断是否为系统 DLL
    if (Is_SystemDLL(dllName.c_str())) {
        std::cout << dllName << std::endl;
        return;
    }

    // 输出非系统 DLL 名称
    SetConsoleColor(FOREGROUND_GREEN);
    std::cout << std::string(depth - 1, '\t') << "[+] " << dllName << std::endl;
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    // 构造 DLL 的完整路径
    std::string dllfilePath = basePath + "\\" + dllName;

    // 获取该 DLL 的导入项列表
    bool flag;
    std::vector<std::string> importedDllList;
    ViewImportedDLLs(dllfilePath.c_str(), importedDllList, flag);

    // 遍历导入的 DLL 列表并递归检查
    for (const auto& importedDll : importedDllList) {
        Recursive_CheckDLL(basePath, importedDll, depth + 1, checkedDlls);  // 增加递归深度
    }
}

// 主函数调用
void Exe_Output(std::filesystem::path filename, std::vector<std::string>& DllList) {
    if (DllList.size())
    {
        if (std::filesystem::exists(filename))
        {
            std::unordered_set<std::string> checkedDlls; // 存储已检测过的 DLL
            std::cout << "Imported DLLs:" << std::endl;
            for (const auto& dll : DllList) {
                Recursive_CheckDLL(filename.parent_path().string(), dll, 1, checkedDlls);
            }
        }
        else
        {
            std::cout << "Error File" << std::endl;
        }

    }
    else
    {
        std::cout << "Nof Find Imported DLLs" << std::endl;
    }

}
void File_Output(std::string filePath, std::vector<std::string>& DllList ,bool is64Bit) {
    if (DllList.size())
    {
        int iNum = 0;
        for (const auto& dll : DllList) {
            if (!Is_SystemDLL(dll.c_str()))
            {
                iNum += 1;
            }

        }
        if (!iNum)
        {
            std::cout << filePath << std::endl;
            return;
        }
        else
        {
            SetConsoleColor(FOREGROUND_GREEN);
            std::cout << "[+] " << filePath << std::endl;
            SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }

        // 获取当前进程路径
        char currentPath[MAX_PATH];
        GetModuleFileNameA(NULL, currentPath, MAX_PATH);
        std::filesystem::path currentDir = std::filesystem::path(currentPath).parent_path();
        std::filesystem::path targetDir;

        std::filesystem::path path(filePath);

        std::string ExeDir = std::to_string(iNum) + "-" + path.stem().string();
        if (is64Bit)
        {
            targetDir = currentDir / "Eyebin" / "x64" / ExeDir;
        }
        else
        {
            targetDir = currentDir / "Eyebin" / "x86" / ExeDir;
        }
        // 创建目标文件夹
        if (!std::filesystem::exists(targetDir)) {
            std::filesystem::create_directories(targetDir);
        }

        std::string ExeName = path.filename().string(); // 获取文件名
        std::filesystem::copy_file(filePath, targetDir / ExeName, std::filesystem::copy_options::overwrite_existing);
        // 移动DLL文件并记录名称
        std::filesystem::create_directories(targetDir / "infos");
        std::ofstream dllNamesFile(targetDir / "infos" / "Info.txt");
        dllNamesFile << filePath << std::endl;
        std::filesystem::path sourceDir = std::filesystem::path(filePath).parent_path();
        bool rename = false;
        for (const auto& dll : DllList) {
            std::filesystem::path sourceDllPath = sourceDir / dll; //源文件路径
            std::filesystem::path targetDllPath = targetDir / dll; //bin路径
            if (!Is_SystemDLL(dll.c_str()))
            {
                dllNamesFile << "[+] " << dll << std::endl;
                if (std::filesystem::exists(sourceDllPath)) {
                    if (std::filesystem::copy_file(sourceDllPath, targetDllPath, std::filesystem::copy_options::overwrite_existing))
                    {
                        std::string txtFile = (targetDir / "infos" / ((std::filesystem::path)dll).stem()).string() + ".txt";
                        EchoFunc(targetDllPath.string().c_str(), txtFile,false);

                        std::vector<std::string> DllList1;
                        bool flag1;
                        int a = 0;
                        ViewImportedDLLs(targetDllPath.string().c_str(), DllList1, flag1);
                        for (const auto& dll1 : DllList1) {
                            if (!Is_SystemDLL(dll1.c_str())) {
                                a += 1;
                            }
                        }
                        if (a != 0)
                        {
                            rename = true;
                            dllNamesFile << "\t[*] 存在嵌套调用其他dll，推荐查看dll的所有嵌套调用的dll名称: \n\t[ ZeroEye.exe -i \"Eyebin\\" << (is64Bit ? "x64" : "x86") << "\\" << ExeDir << " #\\" << dll << "\" ]" << std::endl;
                        }

                    }
                    

                }
            }
            else
            {
                dllNamesFile << dll << std::endl;
            }


        }
        dllNamesFile.close();
        if (rename)
        {
            std::string newDirName =  ExeDir + " #";
            RenameDirectory(targetDir, newDirName);
        }
        bool hasExe = false;
        bool hasDll = false;


        // 遍历目录并检查文件
        for (const auto& entry : std::filesystem::directory_iterator(targetDir)) {
            if (entry.is_regular_file()) {
                std::string filePath = entry.path().string();

                // 使用 strstr 检查文件路径中是否包含 ".exe" 或 ".dll"
                if (strstr(filePath.c_str(), ".exe") != nullptr) {
                    hasExe = true;
                }
                else if (strstr(filePath.c_str(), ".dll") != nullptr) {
                    hasDll = true;
                }
            }
        }

        // 根据条件决定是否删除目录
        if (hasExe && hasDll) {
            
        }
        else {
            std::filesystem::remove_all(targetDir);
        }


    }



}
bool hasReadPermission(const std::string& path) {
    struct _stat fileInfo;
    if (_stat(path.c_str(), &fileInfo) != 0) {
        if (errno == EACCES) {

            SetConsoleColor(FOREGROUND_RED);
            std::cerr << "[-] Permission denied: " << path << std::endl;
            SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else {

            SetConsoleColor(FOREGROUND_RED);
            std::cerr << "[-] Unable to access path: " << path << " (Error code: " << errno << ")" << std::endl;
            SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        return false;
    }
    return true;
}
void getFiles_and_view(const std::string& path) {
    if (!hasReadPermission(path)) {
        return;  // 如果没有访问权限，直接返回
    }

    intptr_t hFile = 0;
    struct _finddata_t fileinfo;

    std::string searchPath = path + "\\*";

    if ((hFile = _findfirst(searchPath.c_str(), &fileinfo)) == -1) {
        SetConsoleColor(FOREGROUND_RED);
        std::cerr << "[-] Failed to open directory: " << path << std::endl;
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        return;
    }
    char currentPath[MAX_PATH];
    GetModuleFileNameA(NULL, currentPath, MAX_PATH);
    do {
        if (fileinfo.attrib & _A_SUBDIR) {
            // 过滤掉 "." 和 ".." 目录
            if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {
                // 排除隐藏的系统目录，例如 "$Recycle.Bin"
                if (fileinfo.name[0] != '$') {
                    std::string subdirPath = path + "\\" + fileinfo.name;
                    if (strstr(path.c_str(), "Eyebin") == nullptr)
                    {
                        getFiles_and_view(subdirPath);
                    }
                    
                }
            }
        }
        else {
            std::string fullPath = path + "\\" + fileinfo.name;

            if (fullPath.size() > 4 && fullPath.substr(fullPath.size() - 4) == ".exe") {
                try {
                    std::vector<std::string> DllList;
                    bool is64Bit;
                    ViewImportedDLLs(fullPath.c_str(), DllList, is64Bit);
                    if (isSign)
                    {
                        if (IsFileSigned(fullPath.c_str())) {
                            File_Output(fullPath.c_str(), DllList,is64Bit);
                        }
                    }
                    else
                    {
                        File_Output(fullPath.c_str(), DllList,is64Bit);
                    }


                }
                catch (const std::exception& e) {
                    continue;

                }
                catch (...) {
                    SetConsoleColor(FOREGROUND_RED);
                    std::cerr << "[-] Unknown error occurred while processing file: " << fullPath << std::endl;
                    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

                }
            }
        }
    } while (_findnext(hFile, &fileinfo) == 0);

    _findclose(hFile);
}

