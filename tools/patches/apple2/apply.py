# The K4532's Apple ][ sidebar in sa2 (AppleWin's SDL2 frontend), run from
# the source tree by build-apple2.sh.  Each edit must find its anchor, or
# the build stops.
#   sdlrendererframe.cpp  includes k4510apple2.cpp; with placement=left the
#                         frame is drawn by k4510a2_present (the picture
#                         flush left, the sidebar beside it)
#   sdlframe.cpp          SDL's events go to k4510a2_event first (the mouse
#                         over the sidebar is the sidebar's)
import os, shutil, sys
here = os.path.dirname(os.path.abspath(__file__))
D = "source/frontends/sdl/"
shutil.copy(os.path.join(here, "k4510apple2.cpp"), D + "renderer/k4510apple2.cpp")
def edit(path, old, new, count=1):
    s = open(path).read()
    if s.count(old) != count:
        sys.exit(f"apple2/apply.py: {path}: expected {count}x {old!r}, found {s.count(old)}")
    open(path, "w").write(s.replace(old, new))
P = D + "renderer/sdlrendererframe.cpp"
edit(P, '#include <iostream>\n', '#include <iostream>\n#include "frontends/sdl/renderer/k4510apple2.cpp"   // K4532-Personalities: the sidebar\n')
edit(P, "        SDL_UpdateTexture(myTexture.get(), nullptr, myFramebuffer.data(), myPitch);\n",
     "        SDL_UpdateTexture(myTexture.get(), nullptr, myFramebuffer.data(), myPitch);\n"
     "        if (k4510a2_on())\n"
     "        {\n"
     "            k4510a2_present(myRenderer.get(), myTexture.get(), myRect);\n"
     "            SDL_RenderPresent(myRenderer.get());\n"
     "            return;\n"
     "        }\n")
P = D + "sdlframe.cpp"
edit(P, "    void SDLFrame::ProcessSingleEvent(const SDL_Event &e, bool &quit)\n    {\n",
     "    void SDLFrame::ProcessSingleEvent(const SDL_Event &e, bool &quit)\n    {\n"
     "        if (k4510a2_event(e, quit)) // K4532-Personalities: the sidebar's mouse\n"
     "        {\n"
     "            return;\n"
     "        }\n")
edit(P, "namespace sa2\n{\n", "bool k4510a2_event(const SDL_Event &e, bool &quit); // renderer/k4510apple2.cpp\n\nnamespace sa2\n{\n", 1)
# No GL under the renderer (a software one, Xvfb): keep time by the timer
# instead of stopping at "SDL_GL_SetSwapInterval: That operation is not
# supported".
edit(P, "        if (it == intervalsToTry.end())\n        {\n            throw std::runtime_error(decorateSDLError(\"SDL_GL_SetSwapInterval\"));\n",
     "        if (it == intervalsToTry.end())\n        {\n"
     "            myTargetGLSwap = 0; // K4532-Personalities: no GL here: the timer\n"
     "            mySynchroniseWithTimer = true;\n"
     "            return;\n")
edit(P, "        if (interval != current && !setGLSwapInterval(interval))\n        {\n            throw std::runtime_error(decorateSDLError(\"SDL_GL_SetSwapInterval\"));\n",
     "        if (interval != current && !setGLSwapInterval(interval))\n        {\n"
     "            return; // K4532-Personalities: no GL here\n")
print("apple2/apply.py: sa2 patched (placement left, the sidebar)")
