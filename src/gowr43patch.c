/*
 * GoWR 4:3 Patcher
 * ----------------
 * Copyright (c) 2026 DR Films - licensed under CC BY-NC-SA 4.0
 * https://creativecommons.org/licenses/by-nc-sa/4.0/  (see LICENSE)
 *
 * God of War Ragnarok snaps the window's aspect ratio (in "Auto" mode) to the
 * nearest entry of a built-in table: 16:9, 16:10, 17:9, 21:9, 21.5:9, 32:9.
 * A 4:3 window snaps to 16:10, which is why you get black bars at the top and
 * bottom on a 4:3 screen.
 *
 * This patcher replaces the 16:10 entry with 4:3 in all three tables
 * (width/height pairs, ratios, inverse ratios) and renames the menu label from
 * "16:10" to "4:3". With Aspect Ratio = Auto (or "4:3" in the menu) the game
 * then renders a true 4:3 image.
 *
 * - Finds the tables by their byte patterns (not fixed offsets), and only
 *   patches if every pattern is found exactly once.
 * - Saves GoWR.exe.4x3-backup before the first change.
 * - Run it again to undo (restores the original bytes).
 *
 * Build (Windows): zig cc -target x86_64-windows-gnu -O2 gowr43patch.c -o GoWR43-Patcher.exe -lshell32 -ladvapi32
 *        Undo tool: add -DUNDO_BUILD and use -o GoWR43-Undo.exe
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

typedef struct {
    const char   *name;
    unsigned char orig[24];      /* full context pattern, original game */
    unsigned char patched[24];   /* same context, after patch */
    int           len;
} Patch;

/* Each pattern contains the 16:9 entry, the 16:10 entry and the 17:9 entry, so
   it is unique and we only ever touch the middle (16:10) part. */
static const Patch PATCHES[] = {
    {   "width:height pairs (16:10 -> 4:3)",
        /* 16.0 9.0 | 16.0 10.0 | 17.0 9.0 */
        { 0x00,0x00,0x80,0x41, 0x00,0x00,0x10,0x41,
          0x00,0x00,0x80,0x41, 0x00,0x00,0x20,0x41,
          0x00,0x00,0x88,0x41, 0x00,0x00,0x10,0x41 },
        /* 16.0 9.0 | 4.0 3.0 | 17.0 9.0 */
        { 0x00,0x00,0x80,0x41, 0x00,0x00,0x10,0x41,
          0x00,0x00,0x80,0x40, 0x00,0x00,0x40,0x40,
          0x00,0x00,0x88,0x41, 0x00,0x00,0x10,0x41 },
        24 },
    {   "aspect ratios (1.6 -> 1.3333)",
        /* 0.0 | 1.7778 | 1.6 | 1.8889 */
        { 0x00,0x00,0x00,0x00, 0x39,0x8e,0xe3,0x3f, 0xcd,0xcc,0xcc,0x3f, 0x1c,0xc7,0xf1,0x3f },
        { 0x00,0x00,0x00,0x00, 0x39,0x8e,0xe3,0x3f, 0xab,0xaa,0xaa,0x3f, 0x1c,0xc7,0xf1,0x3f },
        16 },
    {   "inverse ratios (0.625 -> 0.75)",
        /* 0.5625 | 0.625 | 0.5294 */
        { 0x00,0x00,0x10,0x3f, 0x00,0x00,0x20,0x3f, 0x88,0x87,0x07,0x3f },
        { 0x00,0x00,0x10,0x3f, 0x00,0x00,0x40,0x3f, 0x88,0x87,0x07,0x3f },
        12 },
    {   "menu label (\"16:10\" -> \"4:3\")",
        { '1','6',':','9',0,0,0,0, '1','6',':','1','0',0,0,0, '1','7',':','9' },
        { '1','6',':','9',0,0,0,0, '4',':','3',0,0,0,0,0,     '1','7',':','9' },
        20 },
};
#define NPATCH (int)(sizeof(PATCHES) / sizeof(PATCHES[0]))

static long find_unique(const unsigned char *buf, long size, const unsigned char *pat, int len, int *count)
{
    long pos = -1;
    *count = 0;
    for (long i = 0; i + len <= size; i++) {
        if (buf[i] == pat[0] && memcmp(buf + i, pat, (size_t)len) == 0) {
            if (*count == 0) pos = i;
            (*count)++;
        }
    }
    return pos;
}

static int read_file(const char *path, unsigned char **buf, long *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *buf = (unsigned char *)malloc((size_t)*size);
    if (!*buf || fread(*buf, 1, (size_t)*size, f) != (size_t)*size) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

static int write_file(const char *path, const unsigned char *buf, long size)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(buf, 1, (size_t)size, f) == (size_t)size;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* returns process exit code; mode: 0 = toggle, 1 = force patch, 2 = force restore */
static int run(const char *exe, const char *backup, int mode)
{
    unsigned char *buf;
    long size;
    if (!read_file(exe, &buf, &size)) {
        printf("ERROR: cannot read %s\n", exe);
        printf("Put this patcher in the game folder (next to GoWR.exe).\n");
        return 1;
    }
    printf("Read %s (%ld bytes)\n\n", exe, size);

    long pos_o[NPATCH], pos_p[NPATCH];
    int n_orig = 0, n_patched = 0, bad = 0;
    for (int i = 0; i < NPATCH; i++) {
        int co, cp;
        pos_o[i] = find_unique(buf, size, PATCHES[i].orig, PATCHES[i].len, &co);
        pos_p[i] = find_unique(buf, size, PATCHES[i].patched, PATCHES[i].len, &cp);
        const char *state;
        if (co == 1 && cp == 0)      { state = "original"; n_orig++; }
        else if (co == 0 && cp == 1) { state = "already 4:3"; n_patched++; }
        else                         { state = "NOT FOUND / ambiguous"; bad++; }
        printf("  %-38s %s\n", PATCHES[i].name, state);
    }
    printf("\n");

    if (bad || (n_orig && n_patched)) {
        printf("ERROR: this GoWR.exe doesn't match what the patcher expects\n");
        printf("(different game version, or partly modified). Nothing was changed.\n");
        free(buf);
        return 2;
    }

    int do_restore = (n_patched == NPATCH);
    if (mode == 1 && do_restore) { printf("Already patched (4:3 is active). Nothing to do.\n"); free(buf); return 0; }
    if (mode == 2 && !do_restore) { printf("GoWR.exe is not patched (already original). Nothing to undo.\n"); free(buf); return 0; }
    if (mode == 0 && do_restore) {
        /* never undo by accident: ask first */
        printf("GoWR.exe is ALREADY PATCHED - 4:3 is active.\n\n");
        printf("Do you want to UNDO the patch (back to 16:10)? Type y and Enter, or just Enter to keep 4:3: ");
        fflush(stdout);
        char ans[16] = { 0 };
        if (!fgets(ans, sizeof ans, stdin) || (ans[0] != 'y' && ans[0] != 'Y')) {
            printf("\nKept. 4:3 stays active.\n");
            free(buf);
            return 0;
        }
    }

    if (!do_restore && !file_exists(backup)) {
        if (!write_file(backup, buf, size)) {
            printf("ERROR: could not write backup %s. Nothing was changed.\n", backup);
            free(buf);
            return 3;
        }
        printf("Backup saved: %s\n", backup);
    }

    for (int i = 0; i < NPATCH; i++) {
        if (do_restore) memcpy(buf + pos_p[i], PATCHES[i].orig, (size_t)PATCHES[i].len);
        else            memcpy(buf + pos_o[i], PATCHES[i].patched, (size_t)PATCHES[i].len);
    }
    if (!write_file(exe, buf, size)) {
        printf("ERROR: could not write %s. Is the game still running?\n", exe);
        free(buf);
        return 3;
    }
    free(buf);

    if (do_restore) {
        printf("DONE: original 16:10 restored. The game is back to normal.\n");
    } else {
        printf("DONE: GoWR.exe now has 4:3 in place of 16:10.\n\n");
        printf("In the game (or settings.ini) use:\n");
        printf("  Display Mode = Windowed  (with the GoWR43 tool)  or your 4:3 desktop\n");
        printf("  Aspect Ratio = Auto  (or \"4:3\", shown where 16:10 used to be)\n\n");
        printf("Running the patcher again will only ask whether you want to undo.\n");
        printf("After a game update, run it again to re-apply.\n");
    }
    return 0;
}

#ifdef _WIN32
static int can_write_here(const char *dir)
{
    char test[MAX_PATH];
    snprintf(test, MAX_PATH, "%s\\gowr43_write_test.tmp", dir);
    FILE *f = fopen(test, "wb");
    if (!f) return 0;
    fclose(f);
    DeleteFileA(test);
    return 1;
}
#endif

int main(int argc, char **argv)
{
#ifdef UNDO_BUILD
    int mode = 2;                 /* GoWR43-Undo.exe: always restores */
#else
    int mode = 0;
#endif
    const char *exe_arg = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--patch") == 0) mode = 1;
        else if (strcmp(argv[i], "--restore") == 0) mode = 2;
        else exe_arg = argv[i];
    }

    char exe[1024], backup[1100];
#ifdef _WIN32
#ifdef UNDO_BUILD
    SetConsoleTitleA("GoWR 4:3 Undo");
#else
    SetConsoleTitleA("GoWR 4:3 Patcher");
#endif
    char dir[MAX_PATH];
    GetModuleFileNameA(NULL, dir, MAX_PATH);
    char *slash = strrchr(dir, '\\');
    if (slash) *slash = 0;
    if (exe_arg) snprintf(exe, sizeof exe, "%s", exe_arg);
    else         snprintf(exe, sizeof exe, "%s\\GoWR.exe", dir);

    /* Program Files needs admin rights: relaunch elevated if we can't write */
    if (!exe_arg && !can_write_here(dir)) {
        BOOL elevated = FALSE;
        HANDLE tok;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
            TOKEN_ELEVATION te; DWORD sz;
            if (GetTokenInformation(tok, TokenElevation, &te, sizeof te, &sz)) elevated = te.TokenIsElevated;
            CloseHandle(tok);
        }
        if (!elevated) {
            char self[MAX_PATH];
            GetModuleFileNameA(NULL, self, MAX_PATH);
            const char *params = mode == 1 ? "--patch" : mode == 2 ? "--restore" : "";
            HINSTANCE r = ShellExecuteA(NULL, "runas", self, params, dir, SW_SHOWNORMAL);
            if ((INT_PTR)r > 32) return 0;   /* elevated copy takes over */
            printf("Administrator rights are needed to change files in Program Files.\n");
        }
    }
#else
    snprintf(exe, sizeof exe, "%s", exe_arg ? exe_arg : "GoWR.exe");
#endif
    snprintf(backup, sizeof backup, "%s.4x3-backup", exe);

#ifdef UNDO_BUILD
    printf("GoWR 4:3 Undo\n=============\n\n");
#else
    printf("GoWR 4:3 Patcher\n================\n\n");
#endif
    int rc = run(exe, backup, mode);

#ifdef _WIN32
    printf("\nPress Enter to close.");
    getchar();
#endif
    return rc;
}
