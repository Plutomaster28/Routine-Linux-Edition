# Installing yaml-cpp

The script engine requires yaml-cpp for parsing YAML syntax.

## Windows (MSYS2 UCRT64)

```bash
pacman -S mingw-w64-ucrt-x86_64-yaml-cpp
```

## Windows (vcpkg)

```powershell
vcpkg install yaml-cpp:x64-windows
```

## Linux (Debian/Ubuntu)

```bash
sudo apt install libyaml-cpp-dev
```

## Linux (Fedora/RHEL)

```bash
sudo dnf install yaml-cpp-devel
```

## macOS (Homebrew)

```bash
brew install yaml-cpp
```

## Building from Source

If your package manager doesn't have yaml-cpp:

```bash
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
mkdir build && cd build
cmake -DYAML_BUILD_SHARED_LIBS=ON ..
cmake --build .
sudo cmake --install .
```

## Verifying Installation

After installation, rebuild the bot:

```bash
cd discord-bot
rm -rf build  # Clean build
cmake -S . -B build
cmake --build build
```

If you see yaml-cpp related errors, cmake will tell you:
- "Found yaml-cpp" - Good to go
- "Could not find yaml-cpp" - Installation failed

## Troubleshooting

### CMake can't find yaml-cpp

Try setting the path explicitly:

```bash
cmake -S . -B build -Dyaml-cpp_DIR=/path/to/yaml-cpp
```

### Linking errors

Make sure you have the shared library (`.so`/`.dll`) not just headers.

### Multiple installations

If you have yaml-cpp in multiple locations, cmake might pick the wrong one. Set:

```bash
export CMAKE_PREFIX_PATH=/path/to/correct/yaml-cpp
```

## Alternative: Header-Only Mode

yaml-cpp can be used header-only (slower compile, no linking):

1. Clone yaml-cpp into your project:
   ```bash
   git clone https://github.com/jbeder/yaml-cpp.git external/yaml-cpp
   ```

2. Add to CMakeLists.txt:
   ```cmake
   add_subdirectory(external/yaml-cpp)
   target_link_libraries(routine yaml-cpp)
   ```

## Verification

Test that yaml-cpp works:

```cpp
#include <yaml-cpp/yaml.h>
#include <iostream>

int main() {
    YAML::Node config = YAML::Load("{name: test}");
    std::cout << config["name"].as<std::string>() << std::endl;
    return 0;
}
```

Compile:
```bash
g++ test.cpp -lyaml-cpp -o test
./test
```

Should output: `test`

---

Once yaml-cpp is installed, the script engine will work!
