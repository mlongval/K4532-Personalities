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
 *   SCREEN         COLOR, GREEN, AMBER or WHITE (AppleWin's video types),
 *                  and TOGGLE between colour and the last monitor
 *   MODEL          ][, ][+, //E or the enhanced //E: a click switches it and
 *                  starts the machine again
 *   DISKS          the disks in ~/personalities/apple2 and its folders;
 *                  click one: it goes in drive 1 and the machine boots it
 *   EJECT RESET QUIT   drive 1 emptied; CTRL-RESET; back to the K4532
 *   KEYS           the Apple's keys on the PC's
 *
 * The choices are kept in ~/personalities/apple2 (screen, model, disk1),
 * which run/apple2 reads.  Interactive by the mouse; the wheel scrolls the
 * list.  Included by sdlrendererframe.cpp (apple2/apply.py); sdlframe.cpp
 * hands it SDL's events first (k4510a2_event). */
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
#include "Core.h"
#include "Disk.h"
#include "Interface.h"
#include "NTSC_CharSet.h"
#include "Registry.h"
#include "Utilities.h"
#include "Video.h"
#include "linux/linuxframe.h"
#include "frontends/sdl/sdlframe.h"

namespace
{
    int a2_on = -1;                                   /* placement=left */
    std::string a2_root, a2_cwd;                      /* ~/personalities/apple2, the list's folder */
    uint32_t *a2_px;                                  /* the sidebar, machine pixels */
    int a2_w, a2_h, a2_k = 1;                         /* its size, and the multiple it is drawn at */
    SDL_Rect a2_area;                                 /* where, in output pixels */
    SDL_Texture *a2_tex;
    SDL_Renderer *a2_ren;
    int a2_mx = -1, a2_my = -1;                       /* the mouse, machine pixels (-1: not over it) */
    int a2_scroll;

    struct Hit { int x, y, w, h, what, arg; };
    std::vector<Hit> a2_hit;
    enum { H_FILE = 1, H_UP, H_DOWN, H_EJECT, H_RESET, H_QUIT, H_SCREEN, H_SCREENTOG, H_MODEL };
    struct File { std::string name; int dir; };       /* dir: 1 a folder, 2 the one above */
    std::vector<File> a2_files;

    /* the case: Apple ][+ beige, its shadow and edge, the lettering, a quieter one, a folder */
    const uint32_t C_BG = 0xd8ccab, C_SHADE = 0xb3a57f, C_LIGHT = 0xf1e8cf, C_TEXT = 0x3b2f22, C_DIM = 0x8c7c5f, C_FOLDER = 0x1f5fa8;
    const uint32_t C_DBODY = 0xcabd99, C_DSHADE = 0x9e906c, C_LED = 0xe83a2a, C_LEDOFF = 0x6a4a3a;
    const uint32_t C_RAINBOW[6] = { 0x61bb46, 0xfdb827, 0xf5821f, 0xe03a3e, 0x963d97, 0x009ddc };

    /* the screens: the word kept, the label, AppleWin's type */
    const char *const SCREEN_WORD[4] = { "color", "green", "amber", "white" };
    const char *const SCREEN_NAME[4] = { "COLOR", "GREEN", "AMBER", "WHITE" };
    const VideoType_e SCREEN_TYPE[4] = { VT_COLOR_IDEALIZED, VT_MONO_GREEN, VT_MONO_AMBER, VT_MONO_WHITE };
    int a2_mono = 1;                                  /* the last monitor, for TOGGLE */
    /* the models */
    const char *const MODEL_WORD[4] = { "ii", "iiplus", "iie", "iieenh" };
    const char *const MODEL_NAME[4] = { "][", "][+", "//E", "//E ENH" };
    const eApple2Type MODEL_TYPE[4] = { A2TYPE_APPLE2, A2TYPE_APPLE2PLUS, A2TYPE_APPLE2E, A2TYPE_APPLE2EENHANCED };

    const char *const KEYS[][2] = {
        { "CTRL+F2", "RESET" }, { "F2", "POWER OFF+ON" }, { "L-ALT", "OPEN APPLE" }, { "R-ALT", "SOLID APPLE" },
        { "F5", "SWAP DRIVES" }, { "F9", "NEXT SCREEN" }, { "PAUSE", "PAUSE" }, { "SHIFT+F2", "QUIT" }, { nullptr, nullptr }
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
    int screen_now()
    {
        const VideoType_e t = GetVideo().GetVideoType();
        for (int i = 0; i < 4; i++) if (SCREEN_TYPE[i] == t) return i;
        return 0;                                     /* another colour type: COLOR */
    }
    void screen_set(int i)
    {
        if (i < 0 || i > 3) return;
        if (i) a2_mono = i;
        GetVideo().SetVideoType(SCREEN_TYPE[i]);
        dynamic_cast<LinuxFrame &>(GetFrame()).ApplyVideoModeChange();
        save("screen", SCREEN_WORD[i]);
        std::cerr << "k4510apple2: the screen: " << SCREEN_WORD[i] << std::endl;
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

    /* ---- drawing, in machine pixels ----------------------------------------- */
    void fill(int x, int y, int w, int h, uint32_t c)
    {
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > a2_w) w = a2_w - x;
        if (y + h > a2_h) h = a2_h - y;
        c |= 0xff000000u;
        for (int j = 0; j < h; j++)
            std::fill(a2_px + (size_t)(y + j) * a2_w + x, a2_px + (size_t)(y + j) * a2_w + x + std::max(w, 0), c);
    }
    void glyph(int x, int y, char ch, uint32_t fg, uint32_t bg, bool rev)
    {
        unsigned char c = (unsigned char)ch;
        if (c >= 'a' && c <= 'z') c -= 32;            /* the ][+ has no lower case */
        if (c < 0x20 || c > 0x5f) c = '?';
        const unsigned char *g = csbits_a2[0][0x80 | c];   /* normal video, as AppleWin decoded the ROM */
        if (rev) std::swap(fg, bg);
        for (int j = 0; j < 8; j++)
            for (int i = 0; i < 8; i++)
                fill(x + i, y + j, 1, 1, i < 7 && (g[j] & (0x40 >> i)) ? fg : bg);
    }
    void text(int col, int row, const std::string &s, uint32_t fg, bool rev = false)
    {
        for (size_t i = 0; i < s.size() && col + (int)i < a2_w / 8; i++) glyph((col + (int)i) * 8, row * 8, s[i], fg, C_BG, rev);
    }
    bool over(int x, int y, int w, int h) { return a2_mx >= x && a2_mx < x + w && a2_my >= y && a2_my < y + h; }
    void hitbox(int x, int y, int w, int h, int what, int arg) { a2_hit.push_back({ x, y, w, h, what, arg }); }
    int button(int col, int row, const std::string &s, int what)
    {
        const int n = (int)s.size();
        text(col, row, s, over(col * 8, row * 8, n * 8, 8) ? C_FOLDER : C_TEXT, true);
        hitbox(col * 8, row * 8, n * 8, 8, what, 0);
        return col + n + 1;
    }
    /* a row of choices: the current one reversed, the one under the mouse in blue */
    void choices(int row, const char *const *names, int n, int cur, int what)
    {
        int col = 1;
        for (int i = 0; i < n; i++)
        {
            const int len = (int)strlen(names[i]);
            const bool on = over(col * 8, row * 8, len * 8, 8);
            text(col, row, names[i], on ? C_FOLDER : i == cur ? C_TEXT : C_DIM, on || i == cur);
            hitbox(col * 8, row * 8, len * 8, 8, what, i);
            col += len + 1;
        }
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
        const int cols = a2_w / 8, rows = a2_h / 8;
        a2_hit.clear();
        fill(0, 0, a2_w, a2_h, C_BG);
        fill(0, 0, 2, a2_h, C_LIGHT);                   /* the case's edge, lit from the left */
        /* the name, and the six stripes of the apple */
        static const char *const title[4] = { "APPLE ][", "APPLE ][+", "APPLE //E", "APPLE //E" };
        const int m = model_now();
        text(1, 1, m < 0 ? "APPLE" : title[m], C_TEXT, true);
        for (int i = 0, x = 11 * 8; i < 6; i++) fill(x, 7 + i * 2, a2_w - x - 8, 2, C_RAINBOW[i]);
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
        /* the screen */
        int r = 12;
        const int sc = screen_now();
        text(1, r, "SCREEN", C_TEXT);
        button(8, r, " TOGGLE ", H_SCREENTOG);
        choices(r + 1, SCREEN_NAME, 4, sc, H_SCREEN);
        r += 3;
        text(1, r, "MODEL", C_TEXT);
        choices(r + 1, MODEL_NAME, 4, model_now(), H_MODEL);
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
            const bool on = over(8, row * 8, a2_w - 16, 8);
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
        /* the mouse: the sidebar draws its own arrow while it is over it */
        if (a2_mx >= 0)
        {
            static const char *const arrow[] = { "B.......", "BB......", "BWB.....", "BWWB....", "BWWWB...", "BWWWWB..",
                                                 "BWWWWWB.", "BWWWBBB.", "BWBWB...", "BB.BWB..", "B...BWB.", "....BB.." };
            for (int j = 0; j < 12; j++)
                for (int i = 0; i < 8; i++)
                    if (arrow[j][i] != '.') fill(a2_mx + i, a2_my + j, 1, 1, arrow[j][i] == 'W' ? 0xffffff : 0x000000);
        }
    }

    void click(int x, int y, bool &quit)
    {
        for (const Hit &h : a2_hit)
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
            case H_SCREENTOG: screen_set(screen_now() ? 0 : a2_mono); break;
            case H_MODEL: model_set(h.arg); break;
            }
            return;
        }
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
        {   /* the monitor TOGGLE goes back to: the saved one if a monitor */
            const std::string f = a2_root + "/screen";
            char b[32] = "";
            if (FILE *fp = fopen(f.c_str(), "r")) { if (fgets(b, sizeof b, fp)) b[strcspn(b, "\r\n")] = 0; fclose(fp); }
            for (int i = 1; i < 4; i++) if (!strcmp(b, SCREEN_WORD[i])) a2_mono = i;
        }
        read_files();
    }
    return a2_on;
}

/* The frame: the Apple's picture (src of tex, upside down as AppleWin keeps
 * it) at full height in 4:3, flush left, and the sidebar in the rest. */
void k4510a2_present(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect &src)
{
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
    if (pw != a2_w || ph != a2_h || r != a2_ren)
    {
        delete[] a2_px;
        if (a2_tex) SDL_DestroyTexture(a2_tex);
        a2_w = pw; a2_h = ph; a2_k = k; a2_ren = r;
        a2_px = new uint32_t[(size_t)pw * ph];
        a2_tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, pw, ph);
        a2_area = { w, 0, pw * k, ph * k };
        std::cerr << "k4510apple2: the panel at " << w << ",0, " << pw << "x" << ph << " machine pixels x" << k
                  << "; the picture " << w << "x" << H << "; model " << (model_now() < 0 ? "other" : MODEL_WORD[model_now()])
                  << ", screen " << SCREEN_WORD[screen_now()] << std::endl;
    }
    render();
    SDL_UpdateTexture(a2_tex, nullptr, a2_px, a2_w * 4);
    SDL_RenderCopy(r, a2_tex, nullptr, &a2_area);
}

/* SDL's events, before the emulator's: the mouse over the sidebar is the
 * sidebar's; true when taken */
bool k4510a2_event(const SDL_Event &e, bool &quit)
{
    if (!k4510a2_on() || !a2_px) return false;
    switch (e.type)
    {
    case SDL_MOUSEMOTION:
    {
        const int x = e.motion.x - a2_area.x, y = e.motion.y - a2_area.y;
        const bool in = x >= 0 && y >= 0 && x < a2_area.w && y < a2_area.h;
        a2_mx = in ? x / a2_k : -1;
        a2_my = in ? y / a2_k : -1;
        SDL_ShowCursor(in ? SDL_DISABLE : SDL_ENABLE);
        return in;
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
        const int x = e.button.x - a2_area.x, y = e.button.y - a2_area.y;
        if (x < 0 || y < 0 || x >= a2_area.w || y >= a2_area.h) return false;
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            if (getenv("K4510_A2_DEBUG")) std::cerr << "k4510apple2: click " << x / a2_k << "," << y / a2_k << std::endl;
            click(x / a2_k, y / a2_k, quit);
        }
        return true;
    }
    case SDL_MOUSEWHEEL:
        if (a2_mx < 0) return false;
        a2_scroll -= e.wheel.y * 3;
        return true;
    }
    return false;
}
