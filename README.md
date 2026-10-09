# PBL2
PBL2 of Cao Khiem and Thanh Binh
![alt text](Store/image.png)

## Build trên macOS

Cài Xcode Command Line Tools, Homebrew và raylib:

```bash
xcode-select --install
brew install raylib pkg-config
```

Build và chạy từ thư mục gốc repository:

```bash
./src/build-macos.sh
```

Chỉ build, không chạy chương trình:

```bash
./src/build-macos.sh --no-run
```

Script macOS dùng raylib native cài bằng Homebrew. Thư mục
`src/Utils/raylib-6.0_win64_mingw-w64` chỉ dành cho build Windows và không dùng
được để link trên macOS.