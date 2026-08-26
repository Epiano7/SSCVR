#ifndef SSCVR_GAME_BUILD_LAYOUT_H
#define SSCVR_GAME_BUILD_LAYOUT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    const char *name;
    uint64_t executable_size;
    const char *executable_sha256;
    size_t gameplay_render_rva;
    size_t world_draw_rva;
    size_t culling_bounds_rva;
} VrGameBuildLayout;

static const unsigned char vr_gameplay_render_prologue[12] = {
    0x48, 0x8b, 0xc4, 0x55, 0x41, 0x54, 0x41, 0x55,
    0x41, 0x56, 0x41, 0x57
};

static const unsigned char vr_world_draw_prologue[18] = {
    0x48, 0x8b, 0xc4, 0x55, 0x57, 0x41, 0x54, 0x41, 0x56,
    0x41, 0x57, 0x48, 0x8d, 0xa8, 0x58, 0xfe, 0xff, 0xff
};

static const VrGameBuildLayout vr_game_build_layouts[] = {
    {
        "frozen v17 test build",
        15170560ULL,
        "40FAFE9AD3AFFA0A7C70254A757F495F613C2C6F73B9EDB609D9B62B1C65BD6D",
        0x1eacc0, 0x316b50, 0xf8a060
    },
    {
        "v1.990 (revision 5)",
        15171072ULL,
        "3FE88430ADFD97C5F32ED2BFC4612FF918C2FE856BB57A3BB72B3BAB7C0CDA1E",
        0x1eace0, 0x316b30, 0xf8b060
    },
    {
        "v1.990 (revision 8)",
        15173632ULL,
        "B5AFB4081D8F13733DA55BBB10DC0751E1D4F4F1C4F3B15EC11308A6CDE4C3A2",
        0x1ead40, 0x316d70, 0xf8b0e8
    }
};

static inline size_t vr_game_build_layout_count(void)
{
    return sizeof(vr_game_build_layouts) / sizeof(vr_game_build_layouts[0]);
}

static inline const VrGameBuildLayout *vr_find_game_executable(uint64_t size,
                                                                const char *sha256)
{
    for (size_t index = 0; index < vr_game_build_layout_count(); ++index) {
        const VrGameBuildLayout *layout = &vr_game_build_layouts[index];
        if (layout->executable_size == size &&
            strcmp(layout->executable_sha256, sha256) == 0)
            return layout;
    }
    return NULL;
}

static inline const VrGameBuildLayout *vr_find_game_memory_layout(
    const unsigned char *image, size_t image_size)
{
    if (!image) return NULL;
    for (size_t index = 0; index < vr_game_build_layout_count(); ++index) {
        const VrGameBuildLayout *layout = &vr_game_build_layouts[index];
        if (layout->gameplay_render_rva > image_size ||
            sizeof(vr_gameplay_render_prologue) > image_size - layout->gameplay_render_rva ||
            layout->world_draw_rva > image_size ||
            sizeof(vr_world_draw_prologue) > image_size - layout->world_draw_rva ||
            layout->culling_bounds_rva > image_size ||
            sizeof(float) * 4 > image_size - layout->culling_bounds_rva)
            continue;
        if (memcmp(image + layout->gameplay_render_rva,
                   vr_gameplay_render_prologue,
                   sizeof(vr_gameplay_render_prologue)) == 0 &&
            memcmp(image + layout->world_draw_rva,
                   vr_world_draw_prologue,
                   sizeof(vr_world_draw_prologue)) == 0)
            return layout;
    }
    return NULL;
}

#endif
