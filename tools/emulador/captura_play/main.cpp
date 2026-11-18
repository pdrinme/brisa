// brisa_captura: roda um ELF no Play! e salva quadros apresentados pelo GS.
// Uso: brisa_captura <elf> <saida_prefixo> <vsync1>[,<vsync2>...] [roteiro]
//   roteiro "botoes": a partir do vsync 60 segura X, L1, R2, cima, triângulo,
//   START e R3, analógico esquerdo em (40,220) e direito em (255,128).
// Precisa de um servidor X (ex.: Xvfb :99 e DISPLAY=:99).
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "AppConfig.h"
#include "PS2VM.h"
#include "PadHandler.h"
#include "StdStream.h"
#include "iop/IopBios.h"
#include "gs/GSH_OpenGL/GSH_OpenGL.h"
#include "opengl/OpenGlDef.h"
// X11 por último: as macros dele (None, Bool, Status) colidem com nomes do Play!.
#include <X11/Xlib.h>
#include <GL/glx.h>

// O Play! deixa a pasta de configuração para cada interface definir.
fs::path CAppConfig::GetBasePath() const { return fs::temp_directory_path() / "brisa_captura_play"; }

static Display* g_disp = nullptr;
static Window g_win = 0;
static GLXFBConfig g_fbc = nullptr;
static std::set<unsigned> g_alvos;
static std::string g_prefixo;
static std::atomic<unsigned> g_vsync{0};
static std::atomic<unsigned> g_salvos{0};
static bool g_roteiro_botoes = false;
const int L = 640, A = 448;

static void salvar_ppm(const std::string& caminho, const std::vector<unsigned char>& rgba) {
    FILE* f = fopen(caminho.c_str(), "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", L, A);
    for (int y = A - 1; y >= 0; y--)  // OpenGL lê de baixo para cima
        for (int x = 0; x < L; x++) fwrite(&rgba[(y * L + x) * 4], 1, 3, f);
    fclose(f);
}

class GshCaptura : public CGSH_OpenGL {
public:
    void InitializeImpl() override {
        typedef GLXContext (*CriarCtx)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
        auto criar = (CriarCtx)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
        int attrs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 2,
                       GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None};
        m_ctx = criar(g_disp, g_fbc, nullptr, True, attrs);
        glXMakeCurrent(g_disp, g_win, m_ctx);
#ifdef USE_GLEW
        glewExperimental = GL_TRUE;
        glewInit();
#endif
        CGSH_OpenGL::InitializeImpl();
        PRESENTATION_PARAMS p;
        p.windowWidth = L;
        p.windowHeight = A;
        p.mode = PRESENTATION_MODE_FILL;
        SetPresentationParams(p);
    }
    void PresentBackbuffer() override {
        unsigned n = ++m_quadros;
        if (g_alvos.count(n)) {
            std::vector<unsigned char> px(L * A * 4);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, L, A, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            salvar_ppm(g_prefixo + "_" + std::to_string(n) + ".ppm", px);
            g_salvos++;
        }
        glXSwapBuffers(g_disp, g_win);
    }
private:
    GLXContext m_ctx = nullptr;
    unsigned m_quadros = 0;
};

class PadRoteiro : public CPadHandler {
public:
    void Update(uint8* ram) override {
        unsigned v = ++g_vsync;
        bool ativo = g_roteiro_botoes && v >= 60;
        using B = PS2::CControllerInfo;
        const B::BUTTON segurar[] = {B::CROSS, B::L1, B::R2, B::DPAD_UP, B::TRIANGLE, B::START, B::R3};
        for (auto* i : m_interfaces) {
            for (int b = B::DPAD_UP; b < B::MAX_BUTTONS; b++) {
                bool p = false;
                for (auto s : segurar) p |= ativo && s == b;
                i->SetButtonState(0, static_cast<B::BUTTON>(b), p, ram);
            }
            i->SetAxisState(0, B::ANALOG_LEFT_X, ativo ? 40 : 0x7F, ram);
            i->SetAxisState(0, B::ANALOG_LEFT_Y, ativo ? 220 : 0x7F, ram);
            i->SetAxisState(0, B::ANALOG_RIGHT_X, ativo ? 255 : 0x7F, ram);
            i->SetAxisState(0, B::ANALOG_RIGHT_Y, 0x7F, ram);
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "uso: %s <elf> <prefixo> <vsync,...> [botoes]\n", argv[0]);
        return 2;
    }
    g_prefixo = argv[2];
    for (char* t = strtok(argv[3], ","); t; t = strtok(nullptr, ",")) g_alvos.insert(atoi(t));
    g_roteiro_botoes = argc > 4 && strcmp(argv[4], "botoes") == 0;
    XInitThreads();
    g_disp = XOpenDisplay(nullptr);
    if (!g_disp) { fprintf(stderr, "sem servidor X (DISPLAY)\n"); return 1; }
    int fb_attrs[] = {GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT,
                      GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, True, None};
    int n = 0;
    GLXFBConfig* fbcs = glXChooseFBConfig(g_disp, DefaultScreen(g_disp), fb_attrs, &n);
    if (!fbcs || n == 0) { fprintf(stderr, "sem FBConfig GLX\n"); return 1; }
    g_fbc = fbcs[0];
    XVisualInfo* vi = glXGetVisualFromFBConfig(g_disp, g_fbc);
    XSetWindowAttributes swa = {};
    swa.colormap = XCreateColormap(g_disp, RootWindow(g_disp, vi->screen), vi->visual, AllocNone);
    g_win = XCreateWindow(g_disp, RootWindow(g_disp, vi->screen), 0, 0, L, A, 0, vi->depth, InputOutput, vi->visual,
                          CWColormap, &swa);
    XMapWindow(g_disp, g_win);
    XFlush(g_disp);

    CPS2VM vm;
    vm.Initialize();
    vm.CreateGSHandler([]() { return new GshCaptura(); });
    vm.CreatePadHandler([]() { return new PadRoteiro(); });
    bool fim = false;
    auto con = vm.m_ee->m_os->OnRequestExit.Connect([&fim]() { fim = true; });
    vm.m_ee->m_os->BootFromFile(argv[1]);
    {
        // stdout do EE (printf) vai para <prefixo>.log
        auto iop = dynamic_cast<CIopBios*>(vm.m_iop->m_bios.get());
        auto saida = new Framework::CStdStream((g_prefixo + ".log").c_str(), "wb");
        iop->GetIoman()->SetFileStream(Iop::CIoman::FID_STDOUT, saida);
    }
    vm.Resume();
    auto inicio = std::chrono::steady_clock::now();
    while (!fim && g_salvos < g_alvos.size()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (std::chrono::steady_clock::now() - inicio > std::chrono::seconds(240)) {
            fprintf(stderr, "tempo esgotado (vsync %u)\n", g_vsync.load());
            break;
        }
    }
    printf("CAPTURA: %u de %zu quadros salvos, vsync %u\n", g_salvos.load(), g_alvos.size(), g_vsync.load());
    vm.Pause();
    vm.DestroyGSHandler();
    vm.Destroy();
    return g_salvos == g_alvos.size() ? 0 : 1;
}
