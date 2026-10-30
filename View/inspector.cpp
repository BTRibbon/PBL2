#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <filesystem>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// ==============================================================================
// PBL2 - DU AN CO SO LAP TRINH: HE THONG QUAN LY CHUOI NHA HANG BEP NHO
// Cong cu: Data Realtime Inspector (Bo giam sat va giai ma du lieu thoi gian thuc)
// Thu muc: View/inspector.cpp -> Bien dich ra View/inspector.exe
// ==============================================================================

// ==============================================================================
// 1. CAU HINH BAN MA HOA (SINH VIEN CO THE DOI TAI DAY THEO YEU CAU CUA THAY)
// ==============================================================================
// Khoa ma hoa XOR mac dinh (0x5A = 90), trung voi CIPHER_KEY trong DataManager.h
unsigned char CIPHER_KEY = 0x5A;

// Ham giai ma 1 byte:
// Mac dinh he thong dung phep toan XOR: decoded = byte ^ CIPHER_KEY.
// Neu thay yeu cau doi ban ma hoa (vi du: Caesar Cipher, phep cong/tru, dich bit, dao bit...)
// thi ban chi can sua cong thuc trong ham decodeByte() nay la xong:
unsigned char decodeByte(unsigned char b) {
    return b ^ CIPHER_KEY;
}

// Ham ma hoa 1 byte (dung khi can ma hoa nguoc lai tu plain text thanh file .dat):
unsigned char encodeByte(unsigned char b) {
    return b ^ CIPHER_KEY;
}

// ==============================================================================
// 2. CAC HAM TIEN ICH
// ==============================================================================

// Lay chuoi thoi gian hien tai dang [HH:MM:SS] de in log ra man hinh console
std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &t);
#else
    localtime_r(&t, &tm_info);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "[%H:%M:%S]", &tm_info);
    return std::string(buffer);
}

// Tu dong tim thu muc data va xac dinh thu muc datainspect
// Ho tro chay file inspector.exe o nhieu vi tri: tu thu muc View/, tu root, hoac tu app/
bool locateDataDirectories(fs::path& dataDir, fs::path& inspectDir) {
    std::vector<fs::path> candidatePaths = {
        "../app/data",       // Khi chay truc tiep tu thu muc View/
        "app/data",          // Khi chay tu thu muc goc cua du an (pbl2/)
        "data",              // Khi chay tu trong thu muc app/
        "../../app/data"     // Du phong duong dan sau hon
    };

    for (const auto& candidate : candidatePaths) {
        if (fs::exists(candidate) && fs::is_directory(candidate)) {
            dataDir = fs::canonical(candidate);
            // Thu muc datainspect se nam cung cap voi thu muc data (trong app/datainspect)
            inspectDir = dataDir.parent_path() / "datainspect";
            return true;
        }
    }
    return false;
}

// Tu dong sua loi byte UTF-8 bi hong (\xB5\xE5\xE7) trong ten chi nhanh neu app.exe cu ghi de
std::string repairCorruptedText(const std::string& content, bool& outWasRepaired) {
    outWasRepaired = false;
    const std::string badSeq = "\xB5\xE5\xE7";
    if (content.find(badSeq) == std::string::npos) {
        return content;
    }

    outWasRepaired = true;
    std::string result;
    result.reserve(content.size());

    size_t start = 0;
    while (start < content.size()) {
        size_t end = content.find('\n', start);
        std::string line = (end == std::string::npos)
            ? content.substr(start)
            : content.substr(start, end - start + 1);

        if (line.find(badSeq) != std::string::npos) {
            // Tim vi tri truong thu 3 (ten chi nhanh nam giua dau '|' thu 2 va thu 3)
            size_t p1 = line.find('|');
            size_t p2 = (p1 != std::string::npos) ? line.find('|', p1 + 1) : std::string::npos;
            size_t p3 = (p2 != std::string::npos) ? line.find('|', p2 + 1) : std::string::npos;

            if (p2 != std::string::npos && p3 != std::string::npos) {
                std::string branchField = line.substr(p2 + 1, p3 - p2 - 1);
                std::string fixedBranch = branchField;

                if (branchField.find("Trung") != std::string::npos) {
                    fixedBranch = "Chi nhánh Trung Tâm";
                } else if (branchField.find("Khoa") != std::string::npos) {
                    fixedBranch = "Chi nhánh Bách Khoa";
                } else if (branchField.find("Nam") != std::string::npos) {
                    fixedBranch = "Chi nhánh Phía Nam";
                } else if (!branchField.empty() && branchField.back() == 'y') {
                    fixedBranch = "Chi nhánh Phía Tây";
                } else if (branchField.find("ng") != std::string::npos) {
                    fixedBranch = "Chi nhánh Phía Đông";
                }

                line = line.substr(0, p2 + 1) + fixedBranch + line.substr(p3);
            }
        }

        result += line;
        if (end == std::string::npos) break;
        start = end + 1;
    }

    return result;
}

// Doc file nhi phan ma hoa (.dat) tu app/data va xuat ra file plain-text (.txt) o app/datainspect
bool inspectFile(const fs::path& srcPath, const fs::path& dstPath, size_t& bytesRead, bool& repairedSource) {
    bytesRead = 0;
    repairedSource = false;

    // Khi app chinh (app.exe) dang ghi du lieu, file co the bi lock tam thoi trong vai miligiay.
    // De chuong trinh khong bi loi hay crash, ta thu mo lai toi da 3 lan cach nhau 50ms:
    std::ifstream inFile;
    for (int retry = 0; retry < 3; ++retry) {
        inFile.open(srcPath, std::ios::binary);
        if (inFile.is_open()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!inFile.is_open()) {
        return false;
    }

    // Doc toan bo file ma hoa vao buffer
    std::vector<unsigned char> rawBuffer((std::istreambuf_iterator<char>(inFile)),
                                         std::istreambuf_iterator<char>());
    inFile.close();

    // Giai ma tung byte theo ham decodeByte
    std::string decodedText;
    decodedText.reserve(rawBuffer.size());
    for (unsigned char rawByte : rawBuffer) {
        decodedText.push_back(static_cast<char>(decodeByte(rawByte)));
    }

    // Tu dong sua triet de neu phat hien chuoi byte loi UTF-8 tu bo nho cu cua app.exe
    bool wasRepaired = false;
    std::string cleanText = repairCorruptedText(decodedText, wasRepaired);

    // Mo file dich de ghi ket qua da giai ma
    std::ofstream outFile(dstPath, std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    outFile.write(cleanText.data(), static_cast<std::streamsize>(cleanText.size()));
    outFile.close();

    // Neu phat hien file .dat goc bi app.exe ghi de byte loi, sua nguoc lai luon file .dat goc
    if (wasRepaired) {
        std::ofstream fixDatFile(srcPath, std::ios::binary);
        if (fixDatFile.is_open()) {
            for (unsigned char c : cleanText) {
                fixDatFile.put(static_cast<char>(encodeByte(c)));
            }
            fixDatFile.close();
            repairedSource = true;
        }
    }

    bytesRead = cleanText.size();
    return true;
}

// ==============================================================================
// 3. HAM CHINH (MAIN)
// ==============================================================================
int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Bat ma hoa UTF-8 cho CMD de khong bi loi font tieng Viet
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // Cho phep truyen khoa tuy chon tu dong lenh neu khong muon build lai file:
    // Vi du: inspector.exe 0x5A hoac inspector.exe 90
    if (argc > 1) {
        std::string argKey = argv[1];
        try {
            if (argKey.rfind("0x", 0) == 0 || argKey.rfind("0X", 0) == 0) {
                CIPHER_KEY = static_cast<unsigned char>(std::stoul(argKey, nullptr, 16));
            } else {
                CIPHER_KEY = static_cast<unsigned char>(std::stoul(argKey));
            }
        } catch (...) {
            std::cout << "[!] Tham so khoa khong hop le, tiep tuc dung khoa mac dinh 0x5A." << std::endl;
        }
    }

    fs::path dataDir;
    fs::path inspectDir;

    // Tim kiem thu muc du lieu app/data
    if (!locateDataDirectories(dataDir, inspectDir)) {
        std::cerr << "[-] Khong tim thay thu muc 'app/data'!" << std::endl;
        std::cerr << "    Vui long kiem tra xem thu muc data co ton tai hay chua." << std::endl;
        std::cout << "\nNhan Enter de thoat...";
        std::cin.get();
        return 1;
    }

    // Tu dong tao thu muc app/datainspect neu chua co san
    try {
        if (!fs::exists(inspectDir)) {
            fs::create_directories(inspectDir);
        }
    } catch (const std::exception& e) {
        std::cerr << "[-] Khong the tao thu muc datainspect: " << e.what() << std::endl;
        return 1;
    }

    // Giao dien console sinh vien moc mac, gon gang, de theo doi khi bao cao
    std::cout << "================================================================" << std::endl;
    std::cout << "          PBL2 - DU AN CO SO LAP TRINH: BEP NHO                 " << std::endl;
    std::cout << "       CONG CU THEO DOI & GIAI MA DU LIEU THOI GIAN THUC        " << std::endl;
    std::cout << "                        (DATA INSPECTOR)                        " << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << " [*] Thu muc doc ma hoa:    " << dataDir.string() << std::endl;
    std::cout << " [*] Thu muc xuat file txt: " << inspectDir.string() << std::endl;
    std::cout << " [*] Khoa giai ma hien tai: 0x" << std::hex << std::uppercase 
              << static_cast<int>(CIPHER_KEY) << std::dec << " (" 
              << static_cast<int>(CIPHER_KEY) << ")" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << " [*] Huong dan: Mo cac file .txt trong thu muc 'app/datainspect/'" << std::endl;
    std::cout << "     de xem noi dung thay doi ngay khi thao tac tren app.exe!" << std::endl;
    std::cout << " [*] Nhan Ctrl + C de dung cong cu theo doi." << std::endl;
    std::cout << "================================================================\n" << std::endl;

    // Bang luu lai moc thoi gian sua doi (last_write_time) cua tung file
    std::map<std::string, fs::file_time_type> lastWriteTracker;

    std::cout << getCurrentTimeString() << " [KHOI DONG] Dang quet va giai ma toan bo file lan dau..." << std::endl;

    // Vong lap giam sat thu muc lien tuc (Real-time Watcher)
    while (true) {
        try {
            if (fs::exists(dataDir) && fs::is_directory(dataDir)) {
                for (const auto& entry : fs::directory_iterator(dataDir)) {
                    if (entry.is_regular_file()) {
                        fs::path filePath = entry.path();
                        std::string fileName = filePath.filename().string();

                        // Bo qua cac file tam .tmp do app tao ra trong luc ghi
                        if (filePath.extension() == ".tmp") {
                            continue;
                        }

                        auto lastWriteTime = entry.last_write_time();

                        // Kiem tra neu file la moi hoac vua bi sua doi noi dung
                        bool needUpdate = false;
                        if (lastWriteTracker.find(fileName) == lastWriteTracker.end()) {
                            needUpdate = true; // File moi xuat hien
                        } else if (lastWriteTracker[fileName] != lastWriteTime) {
                            needUpdate = true; // File vua duoc app.exe ghi de
                        }

                        if (needUpdate) {
                            // Chuyen duoi file sang .txt de thay co mo doc bang Notepad de dang
                            // Vi du: accounts.dat -> accounts.txt
                            std::string outTxtName = filePath.stem().string() + ".txt";
                            fs::path dstTxtPath = inspectDir / outTxtName;

                            size_t bytesProcessed = 0;
                            bool repairedSource = false;
                            if (inspectFile(filePath, dstTxtPath, bytesProcessed, repairedSource)) {
                                if (repairedSource) {
                                    // Cap nhat lai moc thoi gian sau khi sua file .dat goc
                                    lastWriteTime = fs::last_write_time(filePath);
                                }
                                lastWriteTracker[fileName] = lastWriteTime;

                                std::cout << getCurrentTimeString() << " [CAP NHAT] " 
                                          << fileName << " -> " << outTxtName 
                                          << " (" << bytesProcessed << " bytes)" << std::endl;
                            }
                        }
                    }
                }
            }
        } catch (...) {
            // Bo qua ngoai le tam thoi de chuong trinh khong bi vang dot ngot
        }

        // Nghi 500ms giua cac lan quet de khong ton CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
