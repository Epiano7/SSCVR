#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_build_layout.h"

static int test_layout(size_t expected_index)
{
    const size_t image_size = 0x1000000;
    unsigned char *image = (unsigned char *)calloc(image_size, 1);
    if (!image) return 0;

    const VrGameBuildLayout *expected = &vr_game_build_layouts[expected_index];
    memcpy(image + expected->gameplay_render_rva, vr_gameplay_render_prologue,
           sizeof(vr_gameplay_render_prologue));
    memcpy(image + expected->world_draw_rva, vr_world_draw_prologue,
           sizeof(vr_world_draw_prologue));

    const VrGameBuildLayout *found = vr_find_game_memory_layout(image, image_size);
    int passed = found == expected;
    if (passed) {
        image[expected->world_draw_rva] ^= 0xff;
        passed = vr_find_game_memory_layout(image, image_size) == NULL;
    }
    free(image);
    return passed;
}

int main(void)
{
    for (size_t index = 0; index < vr_game_build_layout_count(); ++index) {
        const VrGameBuildLayout *layout = &vr_game_build_layouts[index];
        if (vr_find_game_executable(layout->executable_size,
                                    layout->executable_sha256) != layout ||
            !test_layout(index)) {
            fprintf(stderr, "FAIL game build layout: %s\n", layout->name);
            return 1;
        }
    }
    if (vr_find_game_executable(1, "unknown") != NULL) {
        fputs("FAIL unknown executable accepted\n", stderr);
        return 1;
    }
    puts("PASS exact game build layouts and fail-closed signatures");
    return 0;
}
