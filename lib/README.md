# Routine Extensions

Extensions provide low-level kernel capabilities and hardware acceleration.

## Extension vs Module

- **Modules**: Add commands and features to the bot (live in `modules/`)
- **Extensions**: Provide kernel-level capabilities like SIMD acceleration (live in `lib/`)

## Available Extensions

### simd_extension
- **Type**: SIMD Acceleration
- **Capabilities**: SSE, SSE2
- **Functions**:
  - `simd_memcpy()` - Fast memory copy
  - `simd_mix_audio_f32()` - Mix audio streams
  - `simd_apply_volume_f32()` - Apply volume adjustment

### local_economy_extension
- **Type**: Custom persistent game service
- **Capabilities**: Per-guild accounts, atomic transactions, cooldowns, items,
  leaderboards, and restart persistence
- **Consumer**: `modules/local_economy_module.cpp`
- **Data**: `data/local_economy_v1.db`
- **API**: `include/economy_extension_api.h`

## Building Extensions

```bash
cd lib
mkdir -p build_extensions
cd build_extensions
cmake -G Ninja ..
ninja
cp *.so ..
```

## Using Extensions in Modules

Modules can query and use extension functions:

```c
// In your module code
void* simd_memcpy = kernel_bridge->get_extension_function("simd_memcpy");
if (simd_memcpy) {
    // Use SIMD-accelerated memcpy
    ((void*(*)(void*, const void*, size_t))simd_memcpy)(dest, src, size);
}
```

## Creating New Extensions

1. Create a `.c`, `.cpp`, or `.asm` file in `lib/`
2. Include `extension_interface.h`
3. Implement required exports:
   - `extension_get_info()`
   - `extension_init()`
   - `extension_shutdown()`
   - `extension_get_api()`
4. Build with CMake
5. Extension will be auto-loaded on bot startup
