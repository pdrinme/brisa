// Plataforma PC: tempo simulado (determinístico), controle por roteiro e
// rasterização em software. Usada pela prévia (tools/previa) e pelos testes.
#include "plataforma/plataforma.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rasterizador.h"

namespace {
Rasterizador g_rast;
u64 g_tempo = 0;
u32 g_quadro = 0;
EstadoControle g_controle;
const char* g_png = nullptr;
u32 g_png_quadro = 0;  // 0 = no fim

struct NomeBotao { const char* nome; u16 bit; };
const NomeBotao NOMES[] = {
    {"select", BT_SELECT}, {"l3", BT_L3}, {"r3", BT_R3}, {"start", BT_START}, {"cima", BT_CIMA},
    {"direita", BT_DIREITA}, {"baixo", BT_BAIXO}, {"esquerda", BT_ESQUERDA}, {"l2", BT_L2}, {"r2", BT_R2},
    {"l1", BT_L1}, {"r1", BT_R1}, {"triangulo", BT_TRIANGULO}, {"circulo", BT_CIRCULO}, {"xis", BT_XIS},
    {"quadrado", BT_QUADRADO},
};
}  // namespace

namespace plataforma {

bool iniciar(InfoPlataforma& info, int argc, char** argv) {
    strcpy(info.nome, "HOST");
    strcpy(info.video, "640×448 (PRÉVIA NO PC)");
    strcpy(info.modulos, "nenhum (PC)");
    snprintf(info.caminho_boot, sizeof info.caminho_boot, "%s", argc > 0 ? argv[0] : "?");
    info.memoria_livre = 28u * 1024 * 1024;  // valor ilustrativo; o real vem do PS2
    info.tamanho_programa = 0;
    g_controle = EstadoControle();
    g_controle.modo = ModoControle::ANALOGICO;
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (strncmp(a, "--png=", 6) == 0) g_png = a + 6;
        else if (strncmp(a, "--png-quadro=", 13) == 0) g_png_quadro = static_cast<u32>(atoi(a + 13));
        else if (strcmp(a, "--desconectado") == 0) g_controle.modo = ModoControle::DESCONECTADO;
        else if (strncmp(a, "--lx=", 5) == 0) g_controle.lx = static_cast<u8>(atoi(a + 5));
        else if (strncmp(a, "--ly=", 5) == 0) g_controle.ly = static_cast<u8>(atoi(a + 5));
        else if (strncmp(a, "--rx=", 5) == 0) g_controle.rx = static_cast<u8>(atoi(a + 5));
        else if (strncmp(a, "--ry=", 5) == 0) g_controle.ry = static_cast<u8>(atoi(a + 5));
        else if (strncmp(a, "--segurar=", 10) == 0) {
            char buf[128];
            snprintf(buf, sizeof buf, "%s", a + 10);
            for (char* t = strtok(buf, ","); t; t = strtok(nullptr, ","))
                for (const NomeBotao& n : NOMES)
                    if (strcmp(n.nome, t) == 0) g_controle.botoes |= n.bit;
        }
    }
    return true;
}

void ler_controle(EstadoControle& e) { e = g_controle; }

u64 tempo_us() { return g_tempo; }

void desenhar_e_apresentar(const ListaDesenho2D& l) {
    g_quadro++;
    g_tempo += 16683;  // NTSC: 59,94 campos por segundo
    if (g_png && (g_png_quadro == 0 || g_png_quadro == g_quadro)) g_rast.desenhar(l);
    if (g_png && g_png_quadro == g_quadro) g_rast.salvar_png(g_png);
}

void desenhar_quadro(const Cena3D& cena, Caminho3D caminho, const ListaDesenho2D& hud, Estatisticas3D& est) {
    g_quadro++;
    g_tempo += 33367;  // 30 FPS (NTSC, 2 campos por quadro)
    bool capturar = g_png && (g_png_quadro == 0 || g_png_quadro == g_quadro);
    est = Estatisticas3D();
    // O PC só tem o caminho de referência (o do EE). As estatísticas saem dele
    // mesmo quando não há captura, para os testes e o log.
    struct Contador : SaidaTriangulos {
        void triangulo(const VerticeTela&, const VerticeTela&, const VerticeTela&) override {}
    } contador;
    ConfigTela tela;
    if (capturar) {
        g_rast.iniciar_3d(cena.ceu, cena.neblina.cor, cena.camera.perto);
        pipeline_cena(cena, tela, g_rast, est);
        projetar_sombras(cena, tela, g_rast);
        g_rast.desenhar(hud, false);
    } else {
        pipeline_cena(cena, tela, contador, est);
    }
    (void)caminho;
    if (g_png && g_png_quadro == g_quadro) g_rast.salvar_png(g_png);
}

bool caminho_disponivel(Caminho3D caminho) { return caminho == Caminho3D::EE; }

const u8* conteudo(u32* tamanho) {
    // O PC lê o pacote do disco (no PS2 ele vem embutido no ELF).
    static u8* dados = nullptr;
    static u32 n = 0;
    if (!dados) {
        const char* caminho = getenv("BRISA_PAK");
        if (!caminho) caminho = "build/conteudo/BRISA.PAK";
        FILE* f = fopen(caminho, "rb");
        if (!f) return nullptr;
        fseek(f, 0, SEEK_END);
        long t = ftell(f);
        fseek(f, 0, SEEK_SET);
        dados = static_cast<u8*>(aligned_alloc(64, (static_cast<size_t>(t) + 63) / 64 * 64));
        n = static_cast<u32>(fread(dados, 1, static_cast<size_t>(t), f));
        fclose(f);
    }
    if (tamanho) *tamanho = n;
    return dados;
}

int registrar_textura(const TexturaBrisa& t) {
    if (g_rast.n_texturas >= Rasterizador::MAX_TEXTURAS) return -1;
    g_rast.texturas[g_rast.n_texturas] = t;
    return g_rast.n_texturas++;
}

void log(const char* texto) { fputs(texto, stdout); }

void encerrar() {
    if (g_png && g_png_quadro == 0) g_rast.salvar_png(g_png);
}

}  // namespace plataforma
