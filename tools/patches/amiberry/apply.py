# The K4510's changes to Amiberry 8.3.0's SDL3 renderer (run from the source
# tree by build-amiberry.sh), src/osdep/sdl_renderer.cpp:
#   K4510_PLACEMENT=left  the Amiga picture flush left at full height, the
#                         right of the screen free; K4510_SCALE=integer: whole
#                         multiples.  Native chipset screens only (not RTG).
#   the sidebar           the K4510's scene in that free area
#                         (libk4510side.so via k4510host.c), or the A1200's
#                         own (k4510amiga.c: its LEDs, drives, keys)
# and touchpad.py: a touchpad on the bare console works as the mouse; in
# src/osdep/amiberry.cpp the key events go to the sidebar first (its F11);
# and in src/osdep/gui/main_window.cpp the F12 GUI draws its own pointer on
# the bare console, scaled to the screen.
# SDL3 draws a logical presentation through a texture of its own and lays it
# over the window at present, so the presentation is set aside from our copy
# until the frame has been presented, and given back after.
# Unset: Amiberry as it was.
import os, shutil, sys
here = os.path.dirname(os.path.abspath(__file__))
for f in ("k4510host.c", "k4510host.h"):
    shutil.copy(os.path.join(here, "..", "common", f), "src/osdep/" + f)
shutil.copy(os.path.join(here, "k4510amiga.c"), "src/osdep/k4510amiga.c")
P = "src/osdep/sdl_renderer.cpp"
s = open(P).read()
def edit(old, new, count=1):
    global s
    if s.count(old) != count:
        sys.exit(f"amiberry/apply.py: expected {count}x {old!r}, found {s.count(old)}")
    s = s.replace(old, new)

TOP = r'''#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "k4510host.c"

// K4510 (K4510-Personalities): what the logical presentation was, while it is set
// aside for this frame (k4510_restore), and the sidebar's texture.
static int k4510_restore = 0, k4510_rlw = 0, k4510_rlh = 0;
static SDL_RendererLogicalPresentation k4510_rmode = SDL_LOGICAL_PRESENTATION_DISABLED;
static SDL_Texture* k4510_side_tex = nullptr;
static SDL_Renderer* k4510_side_ren = nullptr;
static int k4510_side_w = 0, k4510_side_h = 0;

static int k4510amiga_draw(SDL_Renderer* r, int ax, int W, int H);   // the A1200's own (k4510amiga.c, at the end)

static void k4510_sidebar(SDL_Renderer* r, int ax, int W, int H)
{
    if (k4510amiga_draw(r, ax, W, H)) return;
    uint32_t* px;
    int w, h, k, fresh;
    if (!k4510host_frame(W - ax, H, &px, &w, &h, &k, &fresh)) return;
    if (!k4510_side_tex || k4510_side_ren != r || k4510_side_w != w || k4510_side_h != h) {
        if (k4510_side_tex && k4510_side_ren == r) SDL_DestroyTexture(k4510_side_tex);
        k4510_side_tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!k4510_side_tex) return;
        SDL_SetTextureScaleMode(k4510_side_tex, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(k4510_side_tex, SDL_BLENDMODE_NONE);
        k4510_side_ren = r; k4510_side_w = w; k4510_side_h = h; fresh = 1;
    }
    if (fresh) SDL_UpdateTexture(k4510_side_tex, nullptr, px, w * 4);
    SDL_FRect d = { static_cast<float>(ax), static_cast<float>((H - h * k) / 2),
        static_cast<float>(w * k), static_cast<float>(h * k) };
    SDL_RenderTexture(r, k4510_side_tex, nullptr, &d);
}

'''
# after the file's own first #include line
i = s.index("#include")
j = s.index("\n", i) + 1
s = s[:j] + TOP + s[j:]

old = "        SDL_RenderTextureRotated(mon->amiga_renderer, m_amiga_texture, &f_crop, &f_quad, 0, nullptr, SDL_FLIP_NONE);\n"
new = r'''        static int k4510_left = -1, k4510_whole = 0;
        if (k4510_left < 0) {
            const char* e = getenv("K4510_PLACEMENT");
            k4510_left = e && !strcmp(e, "left");
            e = getenv("K4510_SCALE");
            k4510_whole = e && !strcmp(e, "integer");
        }
        int k_lw = 0, k_lh = 0;
        SDL_RendererLogicalPresentation k_mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        if (k4510_restore) {
            k_lw = k4510_rlw; k_lh = k4510_rlh; k_mode = k4510_rmode;
        } else {
            SDL_GetRenderLogicalPresentation(mon->amiga_renderer, &k_lw, &k_lh, &k_mode);
        }
        if ((k4510_left || k4510_whole) && !ad->picasso_on && k_lw > 0 && k_lh > 0) {
            SDL_SetRenderLogicalPresentation(mon->amiga_renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
            k4510_restore = 1; k4510_rlw = k_lw; k4510_rlh = k_lh; k4510_rmode = k_mode;
            int W, H;
            SDL_GetRenderOutputSize(mon->amiga_renderer, &W, &H);
            float sc = std::min(static_cast<float>(W) / k_lw, static_cast<float>(H) / k_lh);
            if ((k4510_whole || k_mode == SDL_LOGICAL_PRESENTATION_INTEGER_SCALE) && sc >= 1.0f)
                sc = std::floor(sc);
            const float ox = k4510_left ? 0.0f : (W - k_lw * sc) / 2.0f;
            const float oy = (H - k_lh * sc) / 2.0f;
            SDL_FRect k_quad = { ox + f_quad.x * sc, oy + f_quad.y * sc, f_quad.w * sc, f_quad.h * sc };
            SDL_SetRenderDrawColor(mon->amiga_renderer, 0, 0, 0, 255);
            SDL_RenderClear(mon->amiga_renderer);
            SDL_RenderTextureRotated(mon->amiga_renderer, m_amiga_texture, &f_crop, &k_quad, 0, nullptr, SDL_FLIP_NONE);
            if (k4510_left)
                k4510_sidebar(mon->amiga_renderer, static_cast<int>(std::ceil(ox + k_lw * sc)), W, H);
        } else
        SDL_RenderTextureRotated(mon->amiga_renderer, m_amiga_texture, &f_crop, &f_quad, 0, nullptr, SDL_FLIP_NONE);
'''
edit(old, new)
edit("\tSDL_RenderPresent(mon->amiga_renderer);\n\tif (m_vsync.waitvblankthread_mode <= 0)",
     "\tSDL_RenderPresent(mon->amiga_renderer);\n"
     "\tif (k4510_restore) {\n"
     "\t\tSDL_SetRenderLogicalPresentation(mon->amiga_renderer, k4510_rlw, k4510_rlh, k4510_rmode);\n"
     "\t\tk4510_restore = 0;\n"
     "\t}\n"
     "\tif (m_vsync.waitvblankthread_mode <= 0)")
s += "\n// K4510: the A1200's own sidebar\n#include \"k4510amiga.c\"\n"
open(P, "w").write(s)
print("amiberry/apply.py: placement, sidebar in")
exec(open(os.path.join(here, "touchpad.py")).read())

# the key events: the A1200 sidebar's first (k4510amiga_key: F11, the desktop)
A = "src/osdep/amiberry.cpp"
a = open(A).read()
old = "\t\tcase SDL_EVENT_KEY_DOWN:\n\t\tcase SDL_EVENT_KEY_UP:\n\t\t\thandle_key_event(event);\n"
if a.count(old) != 1:
    sys.exit("amiberry/apply.py: the key events' case not found in amiberry.cpp")
a = a.replace(old, "\t\tcase SDL_EVENT_KEY_DOWN:\n\t\tcase SDL_EVENT_KEY_UP:\n"
                   "\t\t\tif (k4510amiga_key(event)) break;   // K4510: the A1200 sidebar's F11\n"
                   "\t\t\thandle_key_event(event);\n")
j = a.index("static void handle_key_event(const SDL_Event& event)")
a = a[:j] + "int k4510amiga_key(const SDL_Event& e);   // K4510: the A1200 sidebar (k4510amiga.c, in sdl_renderer.cpp)\n" + a[j:]
open(A, "w").write(a)
print("amiberry/apply.py: the sidebar's key in")

# the F12 GUI's pointer on the bare console (Doc, 2026-10-18: "the mouse
# pointer is much too small"): on KMSDRM SDL's pointer is the display
# plane's hardware arrow, the same few pixels on any screen; there ImGui
# draws its own, scaled to the screen's height, and SDL's is hidden
G = "src/osdep/gui/main_window.cpp"
g = open(G).read()
old = "\tstyle.ScaleAllSizes(scaling_factor);\n"
if g.count(old) != 1:
    sys.exit("amiberry/apply.py: ScaleAllSizes not found once in main_window.cpp")
g = g.replace(old, old + """\t// K4510 (K4532-Personalities): the pointer on the bare console, drawn by ImGui at the screen's scale
\tif (const char* k4510_drv = SDL_GetCurrentVideoDriver(); k4510_drv && !strcmp(k4510_drv, "kmsdrm")) {
\t\tint k4510_w = 0, k4510_h = 0;
\t\tSDL_GetWindowSizeInPixels(mon->gui_window, &k4510_w, &k4510_h);
\t\tio.MouseDrawCursor = true;
\t\tstyle.MouseCursorScale = k4510_h >= 1000 ? 3.0f : k4510_h >= 700 ? 2.0f : 1.0f;
\t\tSDL_HideCursor();
\t}
""")
open(G, "w").write(g)
print("amiberry/apply.py: the GUI's pointer in")
