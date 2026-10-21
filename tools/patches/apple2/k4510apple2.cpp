/* k4510apple2.cpp -- the Apple ]['s own sidebar (K4532-Personalities,
 * Doc 2026-10-20: "give it the type of sidebar treatment as the Pet (colors,
 * model etc)"): on sa2 (AppleWin's SDL2 frontend, plain renderer) with
 * placement=left, the picture flush left at full height in a 4:3 frame and,
 * in the free area beside it, the machine itself in its case's beige,
 * lettered in its own character ROM (the ][+'s, upper case only), whole
 * characters on an 8x8 grid:
 *
 *   the drive      a Disk II: its IN USE light lit while it turns, the head's
 *                  track; what is in drive 1 and drive 2
 *   SCREEN         COLOR, or a GREEN, AMBER, WHITE or BLUE (white on light
 *                  blue) monitor; TOGGLE-INVERSE swaps the ink and the
 *                  paper of any of them (Doc, 2026-10-21)
 *   MODEL          ][, ][+, //E or the enhanced //E: a click switches it and
 *                  starts the machine again
 *   DISKS          the disks in ~/personalities/apple2 and its folders;
 *                  click one: it goes in drive 1 and the machine boots it
 *   EJECT RESET QUIT   drive 1 emptied; CTRL-RESET; back to the K4532
 *   KEYS           the Apple's keys on the PC's
 *
 * F12 opens OPTIONS over the picture (Doc, 2026-10-21: "an F12 menu that
 * allows options for stuff like scanlines"): SCANLINES, PIXELS (sharp or
 * smoothed), DISK (AppleWin's fast disk or the real speed), SOUND, and the
 * SCREEN, INVERSE and MODEL of the sidebar, for the keys: arrows choose,
 * Return starts the machine again as the MODEL chosen, F12 or Esc closes.
 *
 * The monitors and the scanlines are drawn here, not by AppleWin: it shows
 * a white monitor (or colour) without its own 50% scanlines, and each frame
 * is coloured on its way to the screen -- ink and paper, every second line
 * darker.  F9 steps through the five screens.
 *
 * The choices are kept in ~/personalities/apple2 (screen, inverse, model,
 * disk1, scanlines, pixels, disk, sound), which run/apple2 and this read.
 * Interactive by the mouse; the wheel scrolls the list.  Included by
 * sdlrendererframe.cpp (apple2/apply.py); sdlframe.cpp hands it SDL's events
 * first (k4510a2_event). */
#include <SDL.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "Card.h"
#include "CardManager.h"
#include "Common.h"
#include "Configuration/IPropertySheet.h"
#include "Core.h"
#include "Disk.h"
#include "Interface.h"
#include "NTSC_CharSet.h"
#include "Registry.h"
#include "Speaker.h"
#include "Utilities.h"
#include "Video.h"
#include "linux/linuxframe.h"
#include "frontends/sdl/sdlframe.h"

namespace
{
    int a2_on = -1;                                   /* placement=left */
    std::string a2_root, a2_cwd;                      /* ~/personalities/apple2, the list's folder */
    int a2_k = 1;                                     /* the sidebar's multiple */
    SDL_Rect a2_area;                                 /* where, in output pixels */
    SDL_Texture *a2_tex;
    SDL_Renderer *a2_ren;
    int a2_scroll;

    struct Hit { int x, y, w, h, what, arg; };
    /* a surface drawn in machine pixels: the sidebar, or the OPTIONS box */
    struct Canvas
    {
        uint32_t *px = nullptr;
        int w = 0, h = 0;
        int mx = -1, my = -1;                         /* the mouse over it (-1: not) */
        std::vector<Hit> hit;
    };
    Canvas a2_side, a2_box, *cv = &a2_side;           /* cv: the one being drawn */
    enum { H_FILE = 1, H_UP, H_DOWN, H_EJECT, H_RESET, H_QUIT, H_SCREEN, H_INVERSE, H_MODEL, H_OPTION };
    struct File { std::string name; int dir; };       /* dir: 1 a folder, 2 the one above */
    std::vector<File> a2_files;

    /* the case: Apple ][+ beige, its shadow and edge, the lettering, a quieter one, a folder */
    const uint32_t C_BG = 0xd8ccab, C_SHADE = 0xb3a57f, C_LIGHT = 0xf1e8cf, C_TEXT = 0x3b2f22, C_DIM = 0x8c7c5f, C_FOLDER = 0x1f5fa8;
    const uint32_t C_DBODY = 0xcabd99, C_DSHADE = 0x9e906c, C_LED = 0xe83a2a, C_LEDOFF = 0x6a4a3a;
    const uint32_t C_RAINBOW[6] = { 0x61bb46, 0xfdb827, 0xf5821f, 0xe03a3e, 0x963d97, 0x009ddc };

    /* the screens: the word kept, the label, the ink and the paper (AppleWin's
     * own green and amber; blue: white on light blue) */
    const int NSCREEN = 5;
    const char *const SCREEN_WORD[NSCREEN] = { "color", "green", "amber", "white", "blue" };
    const char *const SCREEN_NAME[NSCREEN] = { "COLOR", "GREEN", "AMBER", "WHITE", "BLUE" };
    const uint32_t SCREEN_INK[NSCREEN] = { 0, 0x00c000, 0xff8000, 0xffffff, 0xffffff };
    const uint32_t SCREEN_PAPER[NSCREEN] = { 0, 0x000000, 0x000000, 0x000000, 0x5078dc };
    int a2_screen = 0, a2_inverse = 0;
    /* the models */
    const char *const MODEL_WORD[4] = { "ii", "iiplus", "iie", "iieenh" };
    const char *const MODEL_NAME[4] = { "][", "][+", "//E", "//E ENH" };
    const eApple2Type MODEL_TYPE[4] = { A2TYPE_APPLE2, A2TYPE_APPLE2PLUS, A2TYPE_APPLE2E, A2TYPE_APPLE2EENHANCED };
    /* OPTIONS: its rows, their words kept and their labels */
    enum { O_SCAN, O_PIXELS, O_DISK, O_SOUND, O_SCREEN, O_INVERSE, O_MODEL, NOPT };
    const char *const OPT_LABEL[NOPT] = { "SCANLINES", "PIXELS", "DISK", "SOUND", "SCREEN", "INVERSE", "MODEL" };
    const char *const SCAN_WORD[3] = { "off", "soft", "strong" }, *const SCAN_NAME[3] = { "OFF", "SOFT", "STRONG" };
    const int SCAN_KEEP[3] = { 256, 192, 128 };       /* every second line's brightness, /256 */
    const char *const PIX_WORD[2] = { "sharp", "soft" }, *const PIX_NAME[2] = { "SHARP", "SOFT" };
    const char *const DISK_WORD[2] = { "fast", "real" }, *const DISK_NAME[2] = { "FAST", "REAL" };
    const char *const SOUND_WORD[4] = { "off", "low", "mid", "full" }, *const SOUND_NAME[4] = { "OFF", "LOW", "MID", "FULL" };
    const char *const INV_WORD[2] = { "off", "on" }, *const INV_NAME[2] = { "OFF", "ON" };
    int a2_scan = 2, a2_pixels = 0, a2_disk = 0, a2_sound = -1;   /* -1: AppleWin's own volume */
    bool a2_menu;                                     /* OPTIONS open */
    int a2_msel, a2_mmodel;                           /* its row; the MODEL Return will start */
    int a2_ms = 1;                                    /* its multiple */
    SDL_Rect a2_marea;
    SDL_Texture *a2_mtex;
    std::vector<uint32_t> a2_pic;                     /* the picture, coloured */

    const char *const KEYS[][2] = {
        { "CTRL+F2", "RESET" }, { "F2", "POWER OFF+ON" }, { "L-ALT", "OPEN APPLE" }, { "R-ALT", "SOLID APPLE" },
        { "F5", "SWAP DRIVES" }, { "F9", "NEXT SCREEN" }, { "F12", "OPTIONS" }, { "PAUSE", "PAUSE" }, { "SHIFT+F2", "QUIT" },
        { nullptr, nullptr }
    };

    void save(const char *file, const std::string &v)
    {
        const std::string p = a2_root + "/" + file;
        if (FILE *f = fopen(p.c_str(), "w"))
        {
            fprintf(f, "%s\n", v.c_str());
            fclose(f);
        }
    }
    /* a word kept: its index in words, or def */
    int load(const char *file, const char *const *words, int n, int def)
    {
        const std::string p = a2_root + "/" + file;
        char b[32] = "";
        if (FILE *f = fopen(p.c_str(), "r"))
        {
            if (fgets(b, sizeof b, f)) b[strcspn(b, "\r\n")] = 0;
            fclose(f);
        }
        for (int i = 0; i < n; i++) if (!strcmp(b, words[i])) return i;
        return def;
    }

    Disk2InterfaceCard *disk2()
    {
        CardManager &m = GetCardMgr();
        return m.QuerySlot(SLOT6) == CT_Disk2 ? dynamic_cast<Disk2InterfaceCard *>(m.GetObj(SLOT6)) : nullptr;
    }

    bool disk_ext(const std::string &n)
    {
        static const char *const ext[] = { ".dsk", ".do", ".po", ".nib", ".woz", ".2mg", ".zip", ".gz", nullptr };
        std::string l = n;
        std::transform(l.begin(), l.end(), l.begin(), ::tolower);
        for (int i = 0; ext[i]; i++)
            if (l.size() > strlen(ext[i]) && l.compare(l.size() - strlen(ext[i]), std::string::npos, ext[i]) == 0) return true;
        return false;
    }

    void read_files()
    {
        a2_files.clear();
        a2_scroll = 0;
        if (a2_cwd != a2_root) a2_files.push_back({ "..", 2 });   /* up a folder, not past the personality's */
        if (DIR *d = opendir(a2_cwd.c_str()))
        {
            std::vector<File> f;
            while (dirent *e = readdir(d))
            {
                struct stat st;
                if (e->d_name[0] == '.') continue;
                const std::string p = a2_cwd + "/" + e->d_name;
                if (stat(p.c_str(), &st)) continue;
                if (!S_ISDIR(st.st_mode) && !disk_ext(e->d_name)) continue;
                f.push_back({ e->d_name, S_ISDIR(st.st_mode) ? 1 : 0 });
            }
            closedir(d);
            std::sort(f.begin(), f.end(), [](const File &a, const File &b) {
                if (a.dir != b.dir) return a.dir > b.dir;    /* folders first */
                return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
            });
            a2_files.insert(a2_files.end(), f.begin(), f.end());
        }
    }

    /* ---- the choices ------------------------------------------------------- */
    /* AppleWin draws colour, or a white monitor this colours; never its own scanlines */
    void video_keep()
    {
        Video &v = GetVideo();
        const VideoType_e want = a2_screen ? VT_MONO_WHITE : VT_COLOR_IDEALIZED;
        if (v.GetVideoType() == want && !v.IsVideoStyle(VS_HALF_SCANLINES)) return;
        v.SetVideoType(want);
        v.SetVideoStyle(VideoStyle_e(v.GetVideoStyle() & ~VS_HALF_SCANLINES));
        dynamic_cast<LinuxFrame &>(GetFrame()).ApplyVideoModeChange();
    }
    void screen_set(int i)
    {
        if (i < 0 || i >= NSCREEN) return;
        a2_screen = i;
        video_keep();
        save("screen", SCREEN_WORD[i]);
        std::cerr << "k4510apple2: the screen: " << SCREEN_WORD[i] << std::endl;
    }
    void inverse_set(int on)
    {
        a2_inverse = on ? 1 : 0;
        save("inverse", INV_WORD[a2_inverse]);
        std::cerr << "k4510apple2: inverse: " << INV_WORD[a2_inverse] << std::endl;
    }
    int model_now()
    {
        const eApple2Type t = GetApple2Type();
        for (int i = 0; i < 4; i++) if (MODEL_TYPE[i] == t) return i;
        return -1;
    }
    void model_set(int i)
    {
        if (i < 0 || i > 3 || i == model_now()) return;
        SetApple2Type(MODEL_TYPE[i]);
        REGSAVE(REGVALUE_APPLE2_TYPE, MODEL_TYPE[i]);
        save("model", MODEL_WORD[i]);
        std::cerr << "k4510apple2: the model: " << MODEL_WORD[i] << std::endl;
        GetFrame().Restart();                         /* the new machine, from power-on */
    }
    /* the sound: the speaker's and the Mockingboard's volume, a third at a time
     * (AppleWin's numbers are how much quieter than the most) */
    void sound_apply()
    {
        if (a2_sound < 0) return;
        const int max = (int)GetPropertySheet().GetVolumeMax();
        const uint32_t quieter = (uint32_t)(max - max * a2_sound / 3);
        SpkrSetVolume(quieter, max);
        GetCardMgr().GetMockingboardCardMgr().SetVolume(quieter, max);
    }
    int sound_now()
    {
        if (a2_sound >= 0) return a2_sound;
        const int max = std::max(1, (int)GetPropertySheet().GetVolumeMax());
        return std::min(3, std::max(0, ((max - (int)SpkrGetVolume()) * 3 + max / 2) / max));
    }

    int opt_now(int o)
    {
        switch (o)
        {
        case O_SCAN: return a2_scan;
        case O_PIXELS: return a2_pixels;
        case O_DISK: return a2_disk;
        case O_SOUND: return sound_now();
        case O_SCREEN: return a2_screen;
        case O_INVERSE: return a2_inverse;
        case O_MODEL: return model_now();
        }
        return 0;
    }
    int opt_count(int o)
    {
        static const int n[NOPT] = { 3, 2, 2, 4, NSCREEN, 2, 4 };
        return n[o];
    }
    const char *const *opt_names(int o)
    {
        static const char *const *const n[NOPT] = { SCAN_NAME, PIX_NAME, DISK_NAME, SOUND_NAME, SCREEN_NAME, INV_NAME, MODEL_NAME };
        return n[o];
    }
    void opt_set(int o, int i)
    {
        if (i < 0 || i >= opt_count(o)) return;
        switch (o)
        {
        case O_SCAN: a2_scan = i; save("scanlines", SCAN_WORD[i]); std::cerr << "k4510apple2: scanlines: " << SCAN_WORD[i] << std::endl; break;
        case O_PIXELS: a2_pixels = i; save("pixels", PIX_WORD[i]); std::cerr << "k4510apple2: pixels: " << PIX_WORD[i] << std::endl; break;
        case O_DISK: a2_disk = i; save("disk", DISK_WORD[i]); std::cerr << "k4510apple2: disk: " << DISK_WORD[i] << std::endl; break;
        case O_SOUND: a2_sound = i; sound_apply(); save("sound", SOUND_WORD[i]); std::cerr << "k4510apple2: sound: " << SOUND_WORD[i] << std::endl; break;
        case O_SCREEN: screen_set(i); break;
        case O_INVERSE: inverse_set(i); break;
        case O_MODEL: model_set(i); a2_mmodel = i; break;
        }
    }
    void menu_open(bool open)
    {
        a2_menu = open;
        a2_msel = 0;
        a2_mmodel = std::max(0, model_now());
        a2_box.mx = a2_box.my = -1;
        std::cerr << "k4510apple2: options " << (open ? "open" : "closed") << std::endl;
    }

    void choose(int i)
    {
        if (i < 0 || i >= (int)a2_files.size()) return;
        const File f = a2_files[i];
        if (f.dir == 2)
        {
            a2_cwd.erase(a2_cwd.rfind('/'));
            read_files();
            return;
        }
        const std::string p = a2_cwd + "/" + f.name;
        if (f.dir)
        {
            a2_cwd = p;
            read_files();
            return;
        }
        Disk2InterfaceCard *d = disk2();
        if (!d) return;
        const ImageError_e e = d->InsertDisk(DRIVE_1, p, false, false);
        std::cerr << "k4510apple2: drive 1: " << p << (e == eIMAGE_ERROR_NONE ? "" : " (not a disk it reads)") << std::endl;
        if (e != eIMAGE_ERROR_NONE) return;
        save("disk1", p);
        static_cast<sa2::SDLFrame &>(GetFrame()).FrameResetMachineState();   /* and boot it */
    }

    /* ---- drawing, in machine pixels, on cv ---------------------------------- */
    void fill(int x, int y, int w, int h, uint32_t c)
    {
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > cv->w) w = cv->w - x;
        if (y + h > cv->h) h = cv->h - y;
        c |= 0xff000000u;
        for (int j = 0; j < h; j++)
            std::fill(cv->px + (size_t)(y + j) * cv->w + x, cv->px + (size_t)(y + j) * cv->w + x + std::max(w, 0), c);
    }
    void glyph(int x, int y, char ch, uint32_t fg, uint32_t bg, bool rev)
    {
        unsigned char c = (unsigned char)ch;
        if (c >= 'a' && c <= 'z') c -= 32;            /* the ][+ has no lower case */
        if (c < 0x20 || c > 0x5f) c = '?';
        const unsigned char *g = csbits_a2[0][0x80 | c];   /* normal video, as AppleWin decoded the ROM; bit 0 is the leftmost dot */
        if (rev) std::swap(fg, bg);
        for (int j = 0; j < 8; j++)
            for (int i = 0; i < 8; i++)
                fill(x + i, y + j, 1, 1, i < 7 && (g[j] & (1 << i)) ? fg : bg);
    }
    void text(int col, int row, const std::string &s, uint32_t fg, bool rev = false)
    {
        for (size_t i = 0; i < s.size() && col + (int)i < cv->w / 8; i++) glyph((col + (int)i) * 8, row * 8, s[i], fg, C_BG, rev);
    }
    bool over(int x, int y, int w, int h) { return cv->mx >= x && cv->mx < x + w && cv->my >= y && cv->my < y + h; }
    void hitbox(int x, int y, int w, int h, int what, int arg) { cv->hit.push_back({ x, y, w, h, what, arg }); }
    int button(int col, int row, const std::string &s, int what)
    {
        const int n = (int)s.size();
        text(col, row, s, over(col * 8, row * 8, n * 8, 8) ? C_FOLDER : C_TEXT, true);
        hitbox(col * 8, row * 8, n * 8, 8, what, 0);
        return col + n + 1;
    }
    /* a row of choices from col: the current one reversed, the one under the
     * mouse in blue, mark (when not the current) underlined in blue */
    void choices(int col, int row, const char *const *names, int n, int cur, int what, int base = 0, int mark = -1)
    {
        for (int i = 0; i < n; i++)
        {
            const int len = (int)strlen(names[i]);
            const bool on = over(col * 8, row * 8, len * 8, 8);
            text(col, row, names[i], on || i == mark ? C_FOLDER : i == cur ? C_TEXT : C_DIM, on || i == cur);
            if (i == mark && i != cur && !on) fill(col * 8, row * 8 + 8, len * 8 - 1, 1, C_FOLDER);
            hitbox(col * 8, row * 8, len * 8, 8, what, base + i);
            col += len + 1;
        }
    }
    void rainbow(int x, int y, int w)
    {
        for (int i = 0; i < 6; i++) fill(x, y + i * 2, w, 2, C_RAINBOW[i]);
    }
    /* the mouse: drawn by the surface itself while it is over it */
    void arrow()
    {
        static const char *const a[] = { "B.......", "BB......", "BWB.....", "BWWB....", "BWWWB...", "BWWWWB..",
                                         "BWWWWWB.", "BWWWBBB.", "BWBWB...", "BB.BWB..", "B...BWB.", "....BB.." };
        if (cv->mx < 0) return;
        for (int j = 0; j < 12; j++)
            for (int i = 0; i < 8; i++)
                if (a[j][i] != '.') fill(cv->mx + i, cv->my + j, 1, 1, a[j][i] == 'W' ? 0xffffff : 0x000000);
    }
    /* the Disk II, front on: the case, the door's slot and lever, IN USE */
    void drive(int x, int y, bool lit)
    {
        fill(x, y, 72, 40, C_DBODY);
        fill(x, y + 38, 72, 2, C_DSHADE);
        fill(x + 70, y, 2, 40, C_DSHADE);
        fill(x + 6, y + 12, 52, 5, 0x1a1510);          /* the slot */
        fill(x + 26, y + 18, 12, 10, C_DSHADE);          /* the door's lever */
        fill(x + 6, y + 31, 6, 3, lit ? C_LED : C_LEDOFF);
    }
    std::string fit(std::string s, int n)
    {
        if ((int)s.size() > n) s = s.substr(0, n);
        return s;
    }

    void render()
    {
        cv = &a2_side;
        const int cols = cv->w / 8, rows = cv->h / 8;
        cv->hit.clear();
        fill(0, 0, cv->w, cv->h, C_BG);
        fill(0, 0, 2, cv->h, C_LIGHT);                  /* the case's edge, lit from the left */
        /* the name, and the six stripes of the apple */
        static const char *const title[4] = { "APPLE ][", "APPLE ][+", "APPLE //E", "APPLE //E" };
        const int m = model_now();
        text(1, 1, m < 0 ? "APPLE" : title[m], C_TEXT, true);
        rainbow(11 * 8, 7, cv->w - 11 * 8 - 8);
        /* the drive */
        Disk2InterfaceCard *d = disk2();
        Disk_Status_e s1 = DISK_STATUS_OFF, s2 = DISK_STATUS_OFF;
        if (d) d->GetLightStatus(&s1, &s2);
        drive(8, 24, s1 != DISK_STATUS_OFF || s2 != DISK_STATUS_OFF);
        {   /* the drive starting and stopping, in the log (the tests read it) */
            static int was = 0;
            const int now = s1 != DISK_STATUS_OFF ? 1 : s2 != DISK_STATUS_OFF ? 2 : 0;
            if (now != was && d)
                std::cerr << "k4510apple2: " << (now ? "drive " + std::to_string(now) + " turning" : std::string("drives still"))
                          << ", track " << d->GetCurrentTrack() << std::endl;
            was = now;
        }
        text(11, 3, "DISK II", C_TEXT);
        if (d) text(11, 5, "TRACK " + std::to_string(d->GetCurrentTrack()), C_DIM);
        for (int i = 0; i < 2; i++)
        {
            const std::string n = d && !d->IsDriveEmpty(i) ? d->GetBaseName(i) : "";
            text(1, 9 + i, i ? "D2 " : "D1 ", C_TEXT);
            text(4, 9 + i, n.empty() ? "EMPTY" : fit(n, cols - 5), n.empty() ? C_DIM : C_TEXT);
        }
        /* the screen: TOGGLE-INVERSE lit (reversed) while on, as a chosen screen */
        int r = 12;
        text(1, r, "SCREEN", C_TEXT);
        {
            const std::string t = " TOGGLE-INVERSE ";
            const bool on = over(8 * 8, r * 8, (int)t.size() * 8, 8);
            text(8, r, t, on ? C_FOLDER : a2_inverse ? C_TEXT : C_DIM, on || a2_inverse);
            hitbox(8 * 8, r * 8, (int)t.size() * 8, 8, H_INVERSE, 0);
        }
        choices(1, r + 1, SCREEN_NAME, NSCREEN, a2_screen, H_SCREEN);
        r += 3;
        text(1, r, "MODEL", C_TEXT);
        choices(1, r + 1, MODEL_NAME, 4, model_now(), H_MODEL);
        r += 3;
        /* the keys at the bottom, the buttons above them, the disks between */
        int nkeys = 0;
        while (KEYS[nkeys][0]) nkeys++;
        if (nkeys > rows - r - 10) nkeys = std::max(rows - r - 10, 0);
        const int bottom = rows - nkeys - 3;
        std::string head = " DISKS " + a2_cwd.substr(a2_root.size()) + "/";
        if ((int)head.size() > cols - 2) head = " ..." + head.substr(head.size() - (cols - 6));
        text(1, r, head, C_TEXT, true);
        const int top = r + 1, room = std::max(bottom - top - 1, 1), n = (int)a2_files.size();
        a2_scroll = std::max(0, std::min(a2_scroll, n - room));
        if (!n) text(1, top, "NO DISKS HERE", C_DIM);
        for (int i = 0; i < room && a2_scroll + i < n; i++)
        {
            const int row = top + i, j = a2_scroll + i;
            const File &f = a2_files[j];
            const bool on = over(8, row * 8, cv->w - 16, 8);
            std::string label = f.dir ? f.name + "/" : f.name;
            if (a2_scroll && i == 0) { text(cols - 2, row, "^", C_TEXT); hitbox((cols - 2) * 8, row * 8, 8, 8, H_UP, 0); }
            if (i == room - 1 && j < n - 1) { text(cols - 2, row, "V", C_TEXT); hitbox((cols - 2) * 8, row * 8, 8, 8, H_DOWN, 0); }
            text(1, row, fit(label, cols - 4), on ? C_FOLDER : f.dir ? C_FOLDER : C_TEXT, on);
            hitbox(8, row * 8, (cols - 4) * 8, 8, H_FILE, j);
        }
        {
            int col = 1;
            if (d && !d->IsDriveEmpty(DRIVE_1)) col = button(col, bottom, " EJECT ", H_EJECT);
            col = button(col, bottom, " RESET ", H_RESET);
            button(col, bottom, " QUIT ", H_QUIT);
        }
        for (int i = 0; i < nkeys; i++)
        {
            text(1, rows - nkeys - 1 + i, KEYS[i][0], C_TEXT);
            text(11, rows - nkeys - 1 + i, KEYS[i][1], C_DIM);
        }
        arrow();
    }

    /* OPTIONS: 43 x 14 characters, in the case's beige like the sidebar */
    const int MCOLS = 43, MROWS = 14, MCHOICE = 13;
    void render_menu()
    {
        cv = &a2_box;
        cv->hit.clear();
        fill(0, 0, cv->w, cv->h, C_BG);
        fill(0, 0, cv->w, 2, C_LIGHT);
        fill(0, 0, 2, cv->h, C_LIGHT);
        fill(0, cv->h - 2, cv->w, 2, C_SHADE);
        fill(cv->w - 2, 0, 2, cv->h, C_SHADE);
        text(1, 1, " OPTIONS ", C_TEXT, true);
        rainbow(11 * 8, 7, cv->w - 11 * 8 - 8);
        for (int o = 0; o < NOPT; o++)
        {
            const int row = 3 + o;
            if (o == a2_msel) text(1, row, ">", C_FOLDER);
            text(2, row, OPT_LABEL[o], o == a2_msel ? C_TEXT : C_DIM);
            choices(MCHOICE, row, opt_names(o), opt_count(o), opt_now(o), H_OPTION, o * 16, o == O_MODEL ? a2_mmodel : -1);
        }
        if (a2_msel == O_MODEL && a2_mmodel != model_now())
            text(2, MROWS - 3, std::string("RETURN: START AGAIN AS ") + MODEL_NAME[a2_mmodel], C_FOLDER);
        text(2, MROWS - 2, "ARROWS CHOOSE   F12 OR ESC CLOSES", C_DIM);
        arrow();
    }

    void click(int x, int y, bool &quit)
    {
        for (const Hit &h : cv->hit)
        {
            if (x < h.x || x >= h.x + h.w || y < h.y || y >= h.y + h.h) continue;
            switch (h.what)
            {
            case H_FILE: choose(h.arg); break;
            case H_UP: a2_scroll -= 5; break;
            case H_DOWN: a2_scroll += 5; break;
            case H_EJECT:
                if (Disk2InterfaceCard *d = disk2()) d->EjectDisk(DRIVE_1);
                save("disk1", "");
                std::cerr << "k4510apple2: eject" << std::endl;
                break;
            case H_RESET: CtrlReset(); std::cerr << "k4510apple2: reset" << std::endl; break;
            case H_QUIT: std::cerr << "k4510apple2: quit" << std::endl; quit = true; break;
            case H_SCREEN: screen_set(h.arg); break;
            case H_INVERSE: inverse_set(!a2_inverse); break;
            case H_MODEL: model_set(h.arg); break;
            case H_OPTION: a2_msel = h.arg / 16; opt_set(h.arg / 16, h.arg % 16); break;
            }
            return;
        }
    }

    /* the menu's keys */
    void menu_key(SDL_Keycode k)
    {
        const int n = opt_count(a2_msel), cur = a2_msel == O_MODEL ? a2_mmodel : opt_now(a2_msel);
        switch (k)
        {
        case SDLK_F12:
        case SDLK_ESCAPE: menu_open(false); break;
        case SDLK_UP: a2_msel = (a2_msel + NOPT - 1) % NOPT; a2_mmodel = std::max(0, model_now()); break;
        case SDLK_DOWN: a2_msel = (a2_msel + 1) % NOPT; a2_mmodel = std::max(0, model_now()); break;
        case SDLK_LEFT:
        case SDLK_RIGHT:
        {
            const int i = std::max(0, std::min(n - 1, cur + (k == SDLK_LEFT ? -1 : 1)));
            if (a2_msel == O_MODEL) a2_mmodel = i;    /* the machine starts again on Return only */
            else if (i != cur) opt_set(a2_msel, i);
            break;
        }
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            if (a2_msel == O_MODEL) opt_set(O_MODEL, a2_mmodel);
            else opt_set(a2_msel, (cur + 1) % n);
            break;
        }
    }

    /* the picture on its way to the screen: a monitor's ink on its paper (or
     * the colours), inverse, every second line darker */
    const uint8_t *picture(const SDL_Rect &src, const uint8_t *fb, int pitch)
    {
        const int keep = SCAN_KEEP[a2_scan];
        a2_pic.resize((size_t)src.w * src.h);
        const uint32_t ink = a2_inverse ? SCREEN_PAPER[a2_screen] : SCREEN_INK[a2_screen];
        const uint32_t paper = a2_inverse ? SCREEN_INK[a2_screen] : SCREEN_PAPER[a2_screen];
        int ic[3], pc[3];
        for (int c = 0; c < 3; c++) { ic[c] = (ink >> (16 - 8 * c)) & 0xff; pc[c] = (paper >> (16 - 8 * c)) & 0xff; }
        for (int y = 0; y < src.h; y++)
        {
            const uint32_t *in = (const uint32_t *)(fb + (size_t)(src.y + y) * pitch) + src.x;
            uint32_t *out = a2_pic.data() + (size_t)y * src.w;
            const int f = (y & 1) ? keep : 256;
            for (int x = 0; x < src.w; x++)
            {
                const uint32_t p = in[x];
                int v[3] = { (int)(p >> 16) & 0xff, (int)(p >> 8) & 0xff, (int)p & 0xff };
                if (a2_screen)
                {
                    const int l = std::max(v[0], std::max(v[1], v[2]));
                    for (int c = 0; c < 3; c++) v[c] = pc[c] + (ic[c] - pc[c]) * l / 255;
                }
                else if (a2_inverse)
                    for (int c = 0; c < 3; c++) v[c] = 255 - v[c];
                out[x] = 0xff000000u | (uint32_t)(v[0] * f >> 8) << 16 | (uint32_t)(v[1] * f >> 8) << 8 | (uint32_t)(v[2] * f >> 8);
            }
        }
        return (const uint8_t *)a2_pic.data();
    }

    /* a surface (re)made at w x h, its texture with it */
    void canvas_size(Canvas &c, SDL_Texture *&tex, SDL_Renderer *r, int w, int h)
    {
        if (c.px && w == c.w && h == c.h && r == a2_ren && tex) return;
        delete[] c.px;
        if (tex) SDL_DestroyTexture(tex);
        c.w = w; c.h = h;
        c.px = new uint32_t[(size_t)w * h];
        tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    }
}

/* placement=left: the picture flush left, the rest of the screen free */
bool k4510a2_on()
{
    if (a2_on < 0)
    {
        const char *p = getenv("K4510_PLACEMENT"), *home = getenv("HOME");
        a2_on = p && !strcmp(p, "left");
        a2_root = std::string(home ? home : "") + "/personalities/apple2";
        a2_cwd = a2_root;
        a2_screen = load("screen", SCREEN_WORD, NSCREEN, 0);
        a2_inverse = load("inverse", INV_WORD, 2, 0);
        a2_scan = load("scanlines", SCAN_WORD, 3, 2);   /* strong: as AppleWin's own 50% */
        a2_pixels = load("pixels", PIX_WORD, 2, 0);
        a2_disk = load("disk", DISK_WORD, 2, 0);
        a2_sound = load("sound", SOUND_WORD, 4, -1);
        read_files();
    }
    return a2_on;
}

/* The frame: the Apple's picture (src of tex, upside down as AppleWin keeps
 * it, from fb) at full height in 4:3, flush left, and the sidebar in the
 * rest; OPTIONS over the picture while open. */
void k4510a2_present(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect &src, const uint8_t *fb, int pitch)
{
    static bool first = true;
    if (first) { first = false; sound_apply(); }
    video_keep();
    if (Disk2InterfaceCard *d = disk2())              /* a new machine has a new card: kept as chosen */
        if (d->GetEnhanceDisk() != !a2_disk) d->SetEnhanceDisk(!a2_disk);
    if (!a2_screen && !a2_inverse && SCAN_KEEP[a2_scan] == 256) SDL_UpdateTexture(tex, nullptr, fb, pitch);
    else SDL_UpdateTexture(tex, &src, picture(src, fb, pitch), src.w * 4);
    SDL_SetTextureScaleMode(tex, a2_pixels ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);

    int W, H;
    SDL_RenderSetLogicalSize(r, 0, 0);
    SDL_GetRendererOutputSize(r, &W, &H);
    int w = H * 4 / 3;
    if (W - w < 160) w = W - 160;                     /* a narrow screen: room for the sidebar all the same */
    const SDL_Rect dst = { 0, 0, w, H };
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);
    SDL_RenderCopyEx(r, tex, &src, &dst, 0.0, nullptr, SDL_FLIP_VERTICAL);
    /* the sidebar: as many machine pixels as make it at least 30 characters wide, at a whole multiple */
    const int fw = W - w;
    int k = std::max(1, std::min(fw / 240, H / 400));
    if (k > 1 && fw / k < 200) k--;
    const int pw = fw / k, ph = H / k;
    if (pw != a2_side.w || ph != a2_side.h || r != a2_ren)
    {
        canvas_size(a2_side, a2_tex, r, pw, ph);
        if (a2_mtex) { SDL_DestroyTexture(a2_mtex); a2_mtex = nullptr; }
        a2_ren = r;
        a2_k = k;
        a2_area = { w, 0, pw * k, ph * k };
        /* OPTIONS: as big as fits nine tenths of the picture's width, eight of its height */
        a2_ms = std::max(1, std::min(w * 9 / 10 / (MCOLS * 8), H * 8 / 10 / (MROWS * 8)));
        a2_marea = { (w - MCOLS * 8 * a2_ms) / 2, (H - MROWS * 8 * a2_ms) / 2, MCOLS * 8 * a2_ms, MROWS * 8 * a2_ms };
        std::cerr << "k4510apple2: the panel at " << w << ",0, " << pw << "x" << ph << " machine pixels x" << k
                  << "; the picture " << w << "x" << H << "; model " << (model_now() < 0 ? "other" : MODEL_WORD[model_now()])
                  << ", screen " << SCREEN_WORD[a2_screen] << ", inverse " << INV_WORD[a2_inverse]
                  << "; options at " << a2_marea.x << "," << a2_marea.y << " x" << a2_ms << std::endl;
    }
    render();
    SDL_UpdateTexture(a2_tex, nullptr, a2_side.px, a2_side.w * 4);
    SDL_RenderCopy(r, a2_tex, nullptr, &a2_area);
    if (a2_menu)
    {
        canvas_size(a2_box, a2_mtex, r, MCOLS * 8, MROWS * 8);
        render_menu();
        SDL_UpdateTexture(a2_mtex, nullptr, a2_box.px, a2_box.w * 4);
        SDL_RenderCopy(r, a2_mtex, nullptr, &a2_marea);
    }
    cv = &a2_side;
}

/* SDL's events, before the emulator's: the mouse over the sidebar is the
 * sidebar's; F12 opens OPTIONS, and while it is open the keys and the mouse
 * are its; F9 steps through the screens.  true when taken */
bool k4510a2_event(const SDL_Event &e, bool &quit)
{
    if (!k4510a2_on() || !a2_side.px) return false;
    const auto in = [](const SDL_Rect &a, int x, int y) { return x >= a.x && y >= a.y && x < a.x + a.w && y < a.y + a.h; };
    switch (e.type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        const SDL_Keycode k = e.key.keysym.sym;
        const bool plain = !(e.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT | KMOD_GUI));
        if (a2_menu)
        {
            if (e.type == SDL_KEYDOWN) menu_key(k);
            return true;
        }
        if (plain && (k == SDLK_F12 || k == SDLK_F9))
        {
            if (e.type == SDL_KEYDOWN && !e.key.repeat)
            {
                if (k == SDLK_F12) menu_open(true);
                else screen_set((a2_screen + 1) % NSCREEN);
            }
            return true;
        }
        return false;
    }
    case SDL_TEXTINPUT:
        return a2_menu;
    case SDL_MOUSEMOTION:
    {
        const int x = e.motion.x, y = e.motion.y;
        const bool side = in(a2_area, x, y), box = a2_menu && in(a2_marea, x, y);
        a2_side.mx = side ? (x - a2_area.x) / a2_k : -1;
        a2_side.my = side ? (y - a2_area.y) / a2_k : -1;
        a2_box.mx = box ? (x - a2_marea.x) / a2_ms : -1;
        a2_box.my = box ? (y - a2_marea.y) / a2_ms : -1;
        SDL_ShowCursor(side || box ? SDL_DISABLE : SDL_ENABLE);
        return side || a2_menu;
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
        const int x = e.button.x, y = e.button.y;
        const bool down = e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
        if (a2_menu && in(a2_marea, x, y))
        {
            cv = &a2_box;
            if (down) click((x - a2_marea.x) / a2_ms, (y - a2_marea.y) / a2_ms, quit);
            cv = &a2_side;
            return true;
        }
        if (in(a2_area, x, y))
        {
            if (down)
            {
                if (getenv("K4510_A2_DEBUG")) std::cerr << "k4510apple2: click " << (x - a2_area.x) / a2_k << "," << (y - a2_area.y) / a2_k << std::endl;
                cv = &a2_side;
                click((x - a2_area.x) / a2_k, (y - a2_area.y) / a2_k, quit);
            }
            return true;
        }
        if (a2_menu)                                  /* a click on the picture closes OPTIONS */
        {
            if (down) menu_open(false);
            return true;
        }
        return false;
    }
    case SDL_MOUSEWHEEL:
        if (a2_side.mx < 0) return a2_menu;
        a2_scroll -= e.wheel.y * 3;
        return true;
    }
    return false;
}
