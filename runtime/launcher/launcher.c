#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "game_build_layout.h"

#define PATH_CAPACITY 1024

static char g_build_dir[PATH_CAPACITY];
static char g_game_dir[PATH_CAPACITY];

struct SavedEnvironmentVariable {
    const char *name;
    char *value;
    int existed;
};

static int save_and_set_environment(struct SavedEnvironmentVariable *saved,
                                    const char *name, const char *value)
{
    DWORD required;
    saved->name = name;
    saved->value = NULL;
    saved->existed = 0;
    SetLastError(ERROR_SUCCESS);
    required = GetEnvironmentVariableA(name, NULL, 0);
    if (required) {
        saved->value = (char *)HeapAlloc(GetProcessHeap(), 0, required);
        if (!saved->value || GetEnvironmentVariableA(name, saved->value, required) >= required) {
            if (saved->value) HeapFree(GetProcessHeap(), 0, saved->value);
            saved->value = NULL;
            return 0;
        }
        saved->existed = 1;
    } else if (GetLastError() != ERROR_ENVVAR_NOT_FOUND && GetLastError() != ERROR_SUCCESS) {
        return 0;
    }
    return SetEnvironmentVariableA(name, value) != 0;
}

static void restore_environment(struct SavedEnvironmentVariable *saved)
{
    SetEnvironmentVariableA(saved->name, saved->existed ? saved->value : NULL);
    if (saved->value) HeapFree(GetProcessHeap(), 0, saved->value);
    saved->value = NULL;
}

static int join_path(char *out, size_t capacity, const char *left, const char *right)
{
    int written = snprintf(out, capacity, "%s\\%s", left, right);
    return written > 0 && (size_t)written < capacity;
}

static int parent_path(char *path)
{
    char *slash = strrchr(path, '\\');
    if (!slash) return 0;
    *slash = '\0';
    return 1;
}

static int initialize_paths(void)
{
    DWORD length = GetModuleFileNameA(NULL, g_build_dir, sizeof(g_build_dir));
    if (!length || length >= sizeof(g_build_dir) - 1) return 0;
    if (!parent_path(g_build_dir)) return 0;

    DWORD environment_length = GetEnvironmentVariableA(
        "SSCVR_GAME_DIR", g_game_dir, sizeof(g_game_dir));
    if (environment_length > 0 && environment_length < sizeof(g_game_dir)) return 1;
    if (environment_length >= sizeof(g_game_dir)) return 0;

    if (!join_path(g_game_dir, sizeof(g_game_dir), g_build_dir, "game")) return 0;
    if (GetFileAttributesA(g_game_dir) != INVALID_FILE_ATTRIBUTES) return 1;

    char development_root[PATH_CAPACITY];
    if (strlen(g_build_dir) + 1 > sizeof(development_root)) return 0;
    strcpy(development_root, g_build_dir);
    if (!parent_path(development_root) || !parent_path(development_root)) return 0;
    return join_path(g_game_dir, sizeof(g_game_dir), development_root, "game");
}

static int sha256_file(const char *path, char output[65], uint64_t *size_out)
{
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) { CloseHandle(file); return 0; }
    if (size_out) *size_out = (uint64_t)size.QuadPart;

    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    DWORD object_size = 0, received = 0;
    unsigned char *object = NULL;
    unsigned char digest[32];
    int ok = 0;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0) goto done;
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&object_size,
                          sizeof(object_size), &received, 0) < 0) goto done;
    object = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, object_size);
    if (!object) goto done;
    if (BCryptCreateHash(algorithm, &hash, object, object_size, NULL, 0, 0) < 0) goto done;
    unsigned char buffer[1024 * 1024];
    for (;;) {
        DWORD count = 0;
        if (!ReadFile(file, buffer, sizeof(buffer), &count, NULL)) goto done;
        if (!count) break;
        if (BCryptHashData(hash, buffer, count, 0) < 0) goto done;
    }
    if (BCryptFinishHash(hash, digest, sizeof(digest), 0) < 0) goto done;
    for (int i = 0; i < 32; ++i) sprintf(output + i * 2, "%02X", digest[i]);
    output[64] = '\0';
    ok = 1;
done:
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (object) HeapFree(GetProcessHeap(), 0, object);
    CloseHandle(file);
    return ok;
}

static int verify_game(void)
{
    char executable[PATH_CAPACITY], digest[65];
    uint64_t size = 0;
    if (!join_path(executable, sizeof(executable), g_game_dir, "SkillshotCity.exe") ||
        !sha256_file(executable, digest, &size)) {
        fprintf(stderr, "Could not read isolated SkillshotCity.exe.\n");
        return 0;
    }
    const VrGameBuildLayout *layout = vr_find_game_executable(size, digest);
    if (!layout) {
        fprintf(stderr, "Unsupported game version.\nFound %s (%llu bytes)\nSupported builds:\n",
                digest, (unsigned long long)size);
        for (size_t index = 0; index < vr_game_build_layout_count(); ++index) {
            const VrGameBuildLayout *supported = &vr_game_build_layouts[index];
            fprintf(stderr, "  %s: %s (%llu bytes)\n", supported->name,
                    supported->executable_sha256,
                    (unsigned long long)supported->executable_size);
        }
        return 0;
    }
    const char *required[] = {"data", "steam_api64.dll", "steam_appid.txt", "openal32_x64.dll"};
    for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); ++i) {
        char path[PATH_CAPACITY];
        if (!join_path(path, sizeof(path), g_game_dir, required[i])) return 0;
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr, "Missing isolated game component: %s\n", required[i]);
            return 0;
        }
    }
    printf("Verified isolated Skillshot City %s (%s).\n",
           layout->name, layout->executable_sha256);
    return 1;
}

static int write_marker(const char *name, const char *contents)
{
    char path[PATH_CAPACITY];
    if (!join_path(path, sizeof(path), g_game_dir, name)) return 0;
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    if (contents) fwrite(contents, 1, strlen(contents), file);
    fclose(file);
    return 1;
}

static int write_marker_if_missing(const char *name, const char *contents)
{
    char path[PATH_CAPACITY];
    if (!join_path(path, sizeof(path), g_game_dir, name)) return 0;
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return 1;
    return write_marker(name, contents);
}

static int prepare_runtime(void)
{
    if (!verify_game()) return 0;
    char system_directory[PATH_CAPACITY];
    UINT system_length = GetSystemDirectoryA(system_directory, sizeof(system_directory));
    if (!system_length || system_length >= sizeof(system_directory)) {
        fprintf(stderr, "Could not locate the Windows system directory.\n");
        return 0;
    }

    struct CopySpec { const char *source; const char *destination; int absolute; } files[] = {
        {"opengl32.dll", "opengl32.dll", 0},
        {"opengl32.dll", "opengl32_system.dll", 1},
        {"openxr_loader.dll", "openxr_loader.dll", 0}
    };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        char source[PATH_CAPACITY], destination[PATH_CAPACITY];
        const char *source_root = files[i].absolute ? system_directory : g_build_dir;
        if (!join_path(source, sizeof(source), source_root, files[i].source) ||
            !join_path(destination, sizeof(destination), g_game_dir, files[i].destination)) {
            fprintf(stderr, "Runtime path is too long.\n");
            return 0;
        }
        int copied = 0;
        DWORD copy_error = ERROR_SUCCESS;
        /* The game process can disappear from the process table a fraction
           before Windows releases the injected module's final file handle.
           Allow rapid headset reconnect/relaunch without turning that normal
           teardown window into a spurious launcher failure. */
        for (int attempt = 0; attempt < 30; ++attempt) {
            if (CopyFileA(source, destination, FALSE)) {
                copied = 1;
                break;
            }
            copy_error = GetLastError();
            if (copy_error != ERROR_SHARING_VIOLATION &&
                copy_error != ERROR_LOCK_VIOLATION &&
                copy_error != ERROR_ACCESS_DENIED) break;
            Sleep(100);
        }
        if (!copied) {
            fprintf(stderr, "Could not install %s (Windows error %lu).\n",
                    files[i].destination, copy_error);
            return 0;
        }
    }
    char obsolete_guard[PATH_CAPACITY];
    if (!join_path(obsolete_guard, sizeof(obsolete_guard), g_game_dir,
                   "SkillshotVR-singleplayer-guard.txt")) return 0;
    DeleteFileA(obsolete_guard);
    if (!write_marker("SkillshotVR-openxr.txt", "isolated-vr-runtime\n") ||
        !write_marker("SkillshotVR-config.txt",
                      "# Isolated Quest Link calibration\n"
                      "display_width=2.8000\n"
                      "display_distance=2.2000\n"
                      "eye_alignment=0.0000\n"
                      "stereo_strength=0.0000\n") ||
        !write_marker("SkillshotCityVR-geometry-stereo.txt", "true-geometry-two-pass\n") ||
        !write_marker_if_missing("SkillshotCityVR-geometry.ini",
                      "# Full world-space eye separation; each eye uses half.\n"
                      "world_eye_separation=5.200\n"
                      "world_convergence_distance=3360.000\n"
                      "# Preserve each perspective pass at its own camera distance.\n"
                      "auto_convergence=1\n"
                      "vertical_fov_scale=1.650\n"
                      "# Camera-relative depth scaling around the convergence plane.\n"
                      "depth_exaggeration=2.000\n"
                      "# Bounded positional lean; rotation stays comfort-anchored.\n"
                      "head_translation_scale=160.000\n"
                      "# Draw extra real map geometry around the ordinary camera bounds.\n"
                      "peripheral_cull_scale=1.500\n"
                      "# Fixed HUD: 16:9 proportions inside the near-square eye target.\n"
                      "hud_safe_scale_x=0.660\n"
                      "hud_safe_scale_y=0.400\n"
                      "# Neutral gameplay is a lowered tabletop; headset pose layers on top.\n"
                      "tabletop_pitch_degrees=20.000\n"
                      "tabletop_horizontal_offset=0.000\n"
                      "tabletop_vertical_offset=-320.000\n"
                      "tabletop_pivot_distance=3360.000\n")) {
        fprintf(stderr, "Could not create isolated runtime markers.\n");
        return 0;
    }
    printf("Prepared isolated runtime with normal game network access.\n");
    return 1;
}

static int launch_game_with_activation(const char *mode, int allow_activation)
{
    if (!prepare_runtime()) return 0;
    char executable[PATH_CAPACITY], command[PATH_CAPACITY + 8];
    if (!join_path(executable, sizeof(executable), g_game_dir, "SkillshotCity.exe")) return 0;
    int command_length = snprintf(command, sizeof(command), "\"%s\"", executable);
    if (command_length <= 0 || (size_t)command_length >= sizeof(command)) return 0;
    struct SavedEnvironmentVariable environment[3];
    size_t environment_count = 0;
    if (!save_and_set_environment(&environment[environment_count++],
                                  "SKILLSHOTCITYVR_MODE", mode) ||
        !save_and_set_environment(&environment[environment_count++],
                                  "SteamAppId", "308600") ||
        !save_and_set_environment(&environment[environment_count++],
                                  "SteamGameId", "308600")) {
        while (environment_count) restore_environment(&environment[--environment_count]);
        fprintf(stderr, "Could not prepare the isolated launch environment.\n");
        return 0;
    }
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    DWORD creation_flags = 0;
    if (!allow_activation) {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = SW_SHOWNOACTIVATE;
        /* Keep Steam's normal process relationship intact; only the initial
           show command is non-activating. */
        creation_flags = 0;
    }
    BOOL created = CreateProcessA(executable, command, NULL, NULL, FALSE, creation_flags,
                                  NULL, g_game_dir,
                                  &startup, &process);
    while (environment_count) restore_environment(&environment[--environment_count]);
    if (!created) {
        fprintf(stderr, "Could not launch isolated game (Windows error %lu).\n", GetLastError());
        return 0;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    printf("Launched isolated game in %s mode (%s).\n", mode,
           allow_activation ? "interactive window" : "background test; focus unchanged");
    return 1;
}

static int launch_game(const char *mode)
{
    return launch_game_with_activation(mode, 1);
}

static int launch_headset_observer(void)
{
    const char *candidates[] = {
        "C:\\Program Files\\Meta Horizon\\Support\\oculus-diagnostics\\OculusMirror.exe",
        "C:\\Program Files\\Oculus\\Support\\oculus-diagnostics\\OculusMirror.exe"
    };
    const char *executable = NULL;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        if (GetFileAttributesA(candidates[i]) != INVALID_FILE_ATTRIBUTES) {
            executable = candidates[i];
            break;
        }
    }
    if (!executable) {
        fprintf(stderr, "Meta headset mirror was not found.\n");
        return 0;
    }
    char command[PATH_CAPACITY + 64];
    int length = snprintf(command, sizeof(command),
                          "\"%s\" --RectilinearBothEyes --Size 1920 1080 --IncludeSystemGui --SingleInstance",
                          executable);
    if (length <= 0 || (size_t)length >= sizeof(command)) return 0;
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    if (!CreateProcessA(executable, command, NULL, NULL, FALSE, 0, NULL, NULL,
                        &startup, &process)) {
        fprintf(stderr, "Could not launch Meta headset mirror (Windows error %lu).\n", GetLastError());
        return 0;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    printf("Launched two-eye headset compositor observer.\n");
    return 1;
}

int main(int argc, char **argv)
{
    if (!initialize_paths()) {
        fprintf(stderr, "Could not locate the launcher or isolated game directory.\n");
        return 2;
    }
    const char *command = argc > 1 ? argv[1] : "--help";
    if (strcmp(command, "--verify") == 0) return verify_game() ? 0 : 1;
    if (strcmp(command, "--prepare") == 0) return prepare_runtime() ? 0 : 1;
    if (strcmp(command, "--arm-proof") == 0) {
        if (!verify_game()) return 1;
        if (!write_marker("SkillshotCityVR-geometry-proof.txt", "deliberate-stable-eye-pair\n")) {
            fprintf(stderr, "Could not arm stable geometry proof capture.\n");
            return 1;
        }
        printf("Armed stable geometry proof capture.\n");
        return 0;
    }
    if (strcmp(command, "--recenter-tabletop") == 0) {
        if (!verify_game()) return 1;
        if (!write_marker("SkillshotCityVR-tabletop-recenter.txt", "capture-current-head-pose\n")) {
            fprintf(stderr, "Could not request tabletop recenter.\n");
            return 1;
        }
        printf("Requested tabletop recenter at the current headset pose.\n");
        return 0;
    }
    if (strcmp(command, "--launch-vr") == 0) return launch_game("vr") ? 0 : 1;
    if (strcmp(command, "--launch-vr-background") == 0)
        return launch_game_with_activation("vr", 0) ? 0 : 1;
    if (strcmp(command, "--launch-vr-observed") == 0)
        return launch_headset_observer() && launch_game("vr") ? 0 : 1;
    if (strcmp(command, "--launch-standard") == 0) return launch_game("standard") ? 0 : 1;
    if (strcmp(command, "--launch-hud-preview") == 0)
        return launch_game("hud-preview") ? 0 : 1;
    printf("Skillshot City VR isolated launcher\n"
           "  --verify          Verify the frozen isolated game version\n"
           "  --prepare         Install runtime only into the isolated copy\n"
           "  --arm-proof       Capture the next stable gameplay eye pair\n"
           "  --recenter-tabletop Anchor the world at the current headset pose\n"
           "  --launch-vr       Launch isolated VR mode (manual menus)\n"
           "  --launch-vr-background Launch isolated VR test without taking focus\n"
           "  --launch-vr-observed Launch VR with Meta headset observer\n"
           "  --launch-hud-preview Preview the VR HUD inset on the desktop\n"
           "  --launch-standard Launch isolated standard mode (manual menus)\n");
    return 0;
}
