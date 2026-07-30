# Installing yaml-cpp on Linux

Routine uses `yaml-cpp` for its optional in-process script parser. The normal
installer handles this dependency:

```bash
./install_deps_linux.sh
```

Manual package commands are below for troubleshooting.

## Debian and Ubuntu

```bash
sudo apt update
sudo apt install libyaml-cpp-dev pkg-config
```

## Fedora-family systems

```bash
sudo dnf install yaml-cpp-devel pkgconf-pkg-config
```

## Arch-family systems

```bash
sudo pacman -S yaml-cpp pkgconf
```

## Verify discovery

Routine resolves dependencies through the active Linux `pkg-config` database:

```bash
pkg-config --modversion yaml-cpp
pkg-config --cflags --libs yaml-cpp
```

Then run the normal build:

```bash
./build_linux.sh
```

## Troubleshooting

If CMake reports that `yaml-cpp` is missing:

1. Confirm `pkg-config --modversion yaml-cpp` succeeds.
2. Check that `PKG_CONFIG_PATH` does not point only at an MSYS2 or Windows
   package database.
3. Delete only the relevant disposable build directory and configure again.
4. Prefer the distribution package over mixing a manual `/usr/local`
   installation with packaged dependencies.

Do not copy Windows `.dll` or `.a` files into the Linux build. Routine's
Linux-first dependency resolution is designed to prevent mixed-platform
linkage.

## Current script-engine boundary

Installing `yaml-cpp` enables parsing, but it does not add features beyond
those implemented by Routine. Scripts currently support in-memory
`message.create` automation through built-in `responder` and `log` actions.
See [SCRIPT_SYSTEM.md](SCRIPT_SYSTEM.md).
