# PBL2 - Hệ Thống Quản Lý Bán Hàng & Chuỗi Nhà Hàng (Bếp Nhỏ)
PBL2 of Cao Khiem and Thanh Binh
![alt text](Store/image.png)

## Kiến trúc & Điểm nổi bật
- **100% Cấu trúc dữ liệu tự cài đặt (`src/datastructures/`)**:
  - `String`: Chuỗi động tự quản lý bộ nhớ `char*` (Rule of Five), hỗ trợ UTF-8 tiếng Việt.
  - `Vector<T>`: Mảng động tổng quát tự co giãn dung lượng (Rule of Five).
  - `BranchList`: Danh sách liên kết đơn (`Singly Linked List`) lưu trữ các chi nhánh.
  - `MenuLookup`: Bảng băm (`Hash Table` 31 buckets, hàm băm `FNV-1a`, xử lý đụng độ bằng `Separate Chaining`).
  - `OrderQueue`: Hàng đợi `FIFO` bằng danh sách liên kết đơn (`front_`, `rear_`) quản lý đơn hàng chờ xử lý.
  - `MinHeap<T>`: Cây nhị phân `Min-Heap` phục vụ thuật toán tìm đường ngắn nhất `Dijkstra` và `A*`.
- **Phân cấp Kế thừa & Đa hình 3 cấp (`src/models/Employee.h`)**:
  - `Employee` (Lớp cơ sở) $\rightarrow$ `Manager` (Quản lý chi nhánh) $\rightarrow$ `ChainManager` (Quản lý chuỗi).
- **Đóng gói độc lập & Mã hoá dữ liệu (`app/`)**:
  - Khi build, chương trình xuất file thực thi `app/app.exe` (liên kết tĩnh `-static`) và mã hoá toàn bộ dữ liệu sang nhị phân `app/data/*.dat` (`XOR 0x5A` + băm mật khẩu `Salt + FNV-1a`).
  - Người dùng chỉ cần mở thư mục `app/` và nhấp đúp vào `app.exe` để chạy trực tiếp, đọc/ghi trên `app/data/*.dat` mà không tác động vào `src/`.

## Tài khoản đăng nhập mẫu
- **Giám đốc chuỗi (`ChainManager`)**: `chainmanager` / `chainpass`
- **Quản lý chi nhánh (`Manager`)**: `manager1` / `mgr1pass`
- **Nhân viên (`Employee`)**: `employee0101` / `emp0101pass`

## Hướng dẫn Build

### Trên Windows (PowerShell + MinGW-w64)
Build và chạy từ thư mục gốc:
```powershell
powershell -ExecutionPolicy Bypass -File .\src\build.ps1
```
Chỉ build ra thư mục `app/` (không tự chạy) và làm mới dữ liệu mã hoá `.dat` từ `src/data/*.txt`:
```powershell
powershell -ExecutionPolicy Bypass -File .\src\build.ps1 -NoRun -ResetData
```

### Trên macOS
Cài Xcode Command Line Tools, Homebrew, Python 3 và raylib:
```bash
xcode-select --install
brew install raylib pkg-config python
```
Build và chạy từ thư mục gốc repository (xuất ra `app/app` và `app/data/*.dat`):
```bash
./src/build-macos.sh
```
Chỉ build, không chạy chương trình:
```bash
./src/build-macos.sh --no-run --reset-data
```
