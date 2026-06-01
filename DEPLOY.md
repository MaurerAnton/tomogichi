# Deploying Tomogichi on Librem 5 / PinePhone

## Option 1: Flatpak (recommended)

```bash
# Install flatpak-builder
sudo apt install flatpak-builder

# Add KDE runtime
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.kde.Platform/aarch64/6.7 org.kde.Sdk/aarch64/6.7

# Build
cd qt
flatpak-builder --arch=aarch64 build-flatpak org.tomogichi.qt.json --force-clean

# Install locally
flatpak-builder --user --install --force-clean build-flatpak org.tomogichi.qt.json

# Run
flatpak run org.tomogichi.qt
```

## Option 2: Native build on device

```bash
# On Librem 5 / PinePhone:
sudo apt install cmake g++ qt6-base-dev qt6-declarative-dev kirigami2-dev
cd tomogichi/qt
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tomogichi-qt
```

## Option 3: Cross-compile for aarch64

Requires aarch64 Qt6 sysroot. The CLI-only version can be cross-compiled with:
```bash
aarch64-linux-gnu-g++ -std=c++17 -O2 -Isrc src/core/engine.cpp src/data/storage.cpp src/main.cpp -o tomogichi-aarch64
```
Note: GCC 15+ cross-compiler may have glibc symbol version issues.
Flatpak (Option 1) is the recommended deployment method.

## Testing on desktop

```bash
cd qt/build
./tomogichi-qt
```
