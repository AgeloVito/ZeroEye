
#include "EModule.hpp"


void DisplayHelp() {

    std::cout << "Usage: ZeroEye [options]" << std::endl;
    std::cout << "options:" << std::endl;
    std::cout << "  -h\t����" << std::endl;
    std::cout << "  -i\t<PE  ·��>\t#�г�Exe�ĵ����" << std::endl;
    std::cout << "  -IM\t<PE  ·��>\t#�鿴�����" << std::endl;
    std::cout << "  -EX\t<PE  ·��>\t#�鿴������" << std::endl;


    std::cout << "  -p\t<�ļ�Ŀ¼>\t#�Զ������ļ�·���¿ɽٳ����õİ�����" << std::endl;
    std::cout << "  -s\t<ǩ��У��>\t#��������ǩ��exe����̽��" << std::endl;
    std::cout << "  -d\t<����ģ��>\t#����dllģ��" << std::endl;

    std::cout <<  std::endl;

    std::cout << "example:" << std::endl;
    std::cout << "  ZeroEye.exe -i a.exe\t\t\t#��ʾexe�����" << std::endl;
    std::cout << "  ZeroEye.exe -p c:\\\t\t\t#ɨ��c��������exe" << std::endl;
    std::cout << "  ZeroEye.exe -p c:\\ -s\t\t\t#ɨ��c��������exe,���ҽ�ɨ��������ǩ����" << std::endl;
    std::cout << "  ZeroEye.exe -d a.dll\t\t\t#��ָ��dll����ģ��,����뵱ǰ·��" << std::endl;
    std::cout << "  ZeroEye.exe -IM/-EX a.exe/a.dll\t#�鿴�����/������" << std::endl;

}
int main(int argc, char* argv[]) {
    std::cout << R"(
  _____                        _____                
 |__  /   ___   _ __    ___   | ____|  _   _    ___ 
   / /   / _ \ | '__|  / _ \  |  _|   | | | |  / _ \
  / /_  |  __/ | |    | (_) | | |___  | |_| | |  __/
 /____|  \___| |_|     \___/  |_____|  \__, |  \___|
                                       |___/ Ver`3.3              

    Github:https://github.com/ImCoriander/ZeroEye
    ���ںţ�**�㹥��**
)" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    if (argc < 2) {
        DisplayHelp();
        return 1;
    }
    std::string Command = GetCommandLineA();
    if (Command.find("-h") != std::string::npos)
    {
        DisplayHelp();
        return 0;
    }
    if (Command.find("-s") != std::string::npos)
    {
        isSign = true;
    }


    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-s") == 0 ) {
            //����ռ��
            continue;
        }
        else if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 < argc) {
                std::vector<std::string> DllList;
                bool is64Bit;
                std::filesystem::path filename = argv[++i];
                
                ViewImportedDLLs(filename.string().c_str(), DllList, is64Bit);
                SetConsoleColor(FOREGROUND_GREEN);
                std::cout << "\t\t" << filename.stem().string() << " is " << (is64Bit ? "x64" : "x86") << "\n"  << std::endl;
                SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                Exe_Output(filename, DllList);


            }
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                getFiles_and_view(argv[++i]);
            }
        }
        else if (strcmp(argv[i], "-d") == 0) {


            if (i + 1 < argc) {

                std::string DemoFile = argv[++i];
                std::string txtFile = ((std::filesystem::path)DemoFile).stem().string() + ".txt";
                //
                EchoFunc(DemoFile, txtFile,true);
                SetConsoleColor(FOREGROUND_GREEN);
                std::cout << "\n[+] Successful WriteTo : " << txtFile << std::endl;
                SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);


            }
        }

        else if (strcmp(argv[i], "-IM") == 0) {
            if (i + 1 < argc) {
                ListImportedFunctions(argv[++i]);
            }
        }
        else if (strcmp(argv[i], "-EX") == 0) {
            if (i + 1 < argc) {
                std::vector<std::string> Funclist;
                ListExportedFunctions(argv[++i],false, Funclist);
            }
        }
        else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            DisplayHelp();
            return 1;
        }
    }
    // ��¼����ʱ��
    auto end = std::chrono::high_resolution_clock::now();

    // ��������ʱ��
    std::chrono::duration<double> duration = end - start;
    // ����ʱ��ת��Ϊ���Ӻ���
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    // �������ʱ�䣨�����ʽ��
    std::cout << "\n[*] ��ʱ: " << minutes << "m " << seconds << "s" << std::endl;

    return 0;
}
