/**
 * Example SIMD Extension
 * Provides SIMD-accelerated operations for media processing
 */

#include "../include/extension_interface.h"
#include <string.h>
#include <stdlib.h>

#if defined(_M_X64) || defined(_M_IX86)
#include <intrin.h>
#define ROUTINE_X86_SIMD 1
#elif defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#define ROUTINE_X86_SIMD 1
#else
#define ROUTINE_X86_SIMD 0
#endif

// Extension info
static ExtensionInfo ext_info = {
    .name = "simd_accel",
    .description = "SIMD acceleration for media processing",
    .version = "1.0.0",
    .author = "Routine Team",
    .type = EXT_TYPE_SIMD,
    .capabilities =
#if ROUTINE_X86_SIMD
        EXT_CAP_SSE | EXT_CAP_SSE2,
#else
        EXT_CAP_NONE,
#endif
    .api_version = EXTENSION_API_VERSION
};

// Kernel bridge (set during init)
static const ExtensionKernelBridge* kernel = NULL;

// ============================================================================
// SIMD Accelerated Functions
// ============================================================================

/**
 * Fast memory copy using SSE2
 */
static void* simd_memcpy(void* dest, const void* src, size_t n) {
    if (n < 64) {
        return memcpy(dest, src, n);  // Use standard memcpy for small sizes
    }
    
    char* d = (char*)dest;
    const char* s = (const char*)src;
    
    // Copy 64 bytes at a time using SSE2
#if ROUTINE_X86_SIMD
    size_t blocks = n / 64;
    for (size_t i = 0; i < blocks; i++) {
        __m128i xmm0 = _mm_loadu_si128((__m128i*)(s + 0));
        __m128i xmm1 = _mm_loadu_si128((__m128i*)(s + 16));
        __m128i xmm2 = _mm_loadu_si128((__m128i*)(s + 32));
        __m128i xmm3 = _mm_loadu_si128((__m128i*)(s + 48));
        
        _mm_storeu_si128((__m128i*)(d + 0), xmm0);
        _mm_storeu_si128((__m128i*)(d + 16), xmm1);
        _mm_storeu_si128((__m128i*)(d + 32), xmm2);
        _mm_storeu_si128((__m128i*)(d + 48), xmm3);
        
        s += 64;
        d += 64;
    }
    
    // Copy remaining bytes
    size_t remaining = n % 64;
    if (remaining > 0) {
        memcpy(d, s, remaining);
    }
#else
    memcpy(d, s, n);
#endif
    
    return dest;
}

/**
 * Fast audio mixing (add two float arrays)
 */
static void simd_mix_audio_f32(float* dest, const float* src1, const float* src2, size_t samples) {
    size_t i = 0;
    
    // Process 4 samples at a time using SSE
#if ROUTINE_X86_SIMD
    size_t sse_samples = (samples / 4) * 4;
    for (; i < sse_samples; i += 4) {
        __m128 a = _mm_loadu_ps(src1 + i);
        __m128 b = _mm_loadu_ps(src2 + i);
        __m128 result = _mm_add_ps(a, b);
        _mm_storeu_ps(dest + i, result);
    }
#endif
    
    // Process remaining samples
    for (; i < samples; i++) {
        dest[i] = src1[i] + src2[i];
    }
}

/**
 * Fast volume adjustment
 */
static void simd_apply_volume_f32(float* samples, float volume, size_t count) {
    size_t i = 0;
    
#if ROUTINE_X86_SIMD
    __m128 vol = _mm_set1_ps(volume);  // Broadcast volume to all 4 lanes
    // Process 4 samples at a time
    size_t sse_samples = (count / 4) * 4;
    for (; i < sse_samples; i += 4) {
        __m128 data = _mm_loadu_ps(samples + i);
        __m128 result = _mm_mul_ps(data, vol);
        _mm_storeu_ps(samples + i, result);
    }
#endif
    
    // Process remaining samples
    for (; i < count; i++) {
        samples[i] *= volume;
    }
}

// ============================================================================
// Extension API Implementation
// ============================================================================

static int ext_init(void* kernel_context) {
    (void)kernel_context;
    if (kernel) {
        kernel->log("[SIMD] Extension initialized!");
    }
    return 0;  // Success
}

static void ext_shutdown(void) {
    if (kernel) {
        kernel->log("[SIMD] Extension shutdown");
    }
}

static int ext_has_capability(uint32_t capability) {
    // Runtime CPU feature detection could go here
    // For now, just check against declared capabilities
    return (ext_info.capabilities & capability) ? 1 : 0;
}

static void* ext_get_function(const char* function_name) {
    if (strcmp(function_name, "simd_memcpy") == 0) {
        union {
            void* object;
            void* (*function)(void*, const void*, size_t);
        } conversion = { .function = simd_memcpy };
        return conversion.object;
    }
    if (strcmp(function_name, "simd_mix_audio_f32") == 0) {
        union {
            void* object;
            void (*function)(float*, const float*, const float*, size_t);
        } conversion = { .function = simd_mix_audio_f32 };
        return conversion.object;
    }
    if (strcmp(function_name, "simd_apply_volume_f32") == 0) {
        union {
            void* object;
            void (*function)(float*, float, size_t);
        } conversion = { .function = simd_apply_volume_f32 };
        return conversion.object;
    }
    return NULL;
}

static ExtensionAPI ext_api = {
    .init = ext_init,
    .shutdown = ext_shutdown,
    .has_capability = ext_has_capability,
    .get_function = ext_get_function
};

// ============================================================================
// Required Exports
// ============================================================================

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

EXPORT ExtensionInfo* extension_get_info(void) {
    return &ext_info;
}

EXPORT int extension_init(const ExtensionKernelBridge* bridge) {
    kernel = bridge;
    return ext_api.init ? ext_api.init(NULL) : 0;
}

EXPORT void extension_shutdown(void) {
    if (ext_api.shutdown) {
        ext_api.shutdown();
    }
    kernel = NULL;
}

EXPORT const ExtensionAPI* extension_get_api(void) {
    return &ext_api;
}
