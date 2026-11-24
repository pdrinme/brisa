#include "jogador.h"

#include <math.h>
#include <stdio.h>

bool Jogador::iniciar(const Pacote& pak, BibliotecaModelos& bib, char* erro, int ne) {
    if (!rec_.iniciar(pak, bib, erro, ne)) return false;
    l_.iniciar(&rec_, ID, 0, "VOCÊ");
    reposicionar(Vec3(), 0);
    return true;
}

void Jogador::reposicionar(Vec3 pes, f32 yaw, f32 pitch) {
    cam_ = Camera3P();
    cam_.yaw = fmodf(yaw + 720.0f, 360.0f);
    cam_.pitch = pitch;
    l_.reposicionar(pes, cam_.yaw_rad());
    agachar_ = false;
}

void Jogador::passo(const Controle& c, const MundoColisao& m, Loot& loot, AlvoCombate* alvos, int n, u32 tick, bool bloquear,
                    EventosCombate& ev) {
    const f32 dt = 1.0f / 30.0f;
    const EstadoControle& e = c.estado();
    if (!l_.vivo) {  // eliminado: a câmera fica (ou segue quem está sendo assistido)
        cam_.distancia_extra = 0;
        if (espectado_) {
            cam_.olhar(Controle::eixo(e.rx), Controle::eixo(e.ry), dt);
            cam_.distancia_extra = espectado_->p.queda == NO_ONIBUS ? 9.0f : (espectado_->p.queda != NO_CHAO_NORMAL ? 3.0f : 0.0f);
            cam_.atualizar(espectado_->p.pos, espectado_->p.agachado, m, dt);
        } else {
            cam_.atualizar(l_.p.pos, l_.p.agachado, m, dt);
        }
        return;
    }
    // No ônibus a câmera fica bem atrás; na queda e no planador, um pouco mais longe.
    f32 extra = l_.p.queda == NO_ONIBUS ? 9.0f : (l_.p.queda != NO_CHAO_NORMAL ? 3.0f : 0.0f);
    cam_.distancia_extra += (extra - cam_.distancia_extra) * fminf(dt * 3.0f, 1.0f);
    if (c.pressionou(BT_R3)) agachar_ = !agachar_;  // R3 alterna (Ventania)
    cam_.mirando = c.segurando(BT_L2) && !bloquear;
    cam_.olhar(Controle::eixo(e.rx), Controle::eixo(e.ry), dt);
    ComandoLutador k;
    k.mov.x = Controle::eixo(e.lx);
    k.mov.y = -Controle::eixo(e.ly);  // analógico para cima = frente
    k.mov.yaw = cam_.yaw_rad();
    k.mov.pitch = cam_.pitch * 3.14159265f / 180.0f;  // olhar para baixo na queda = mergulho
    k.mov.pular = c.pressionou(BT_XIS);
    k.mov.correr = c.segurando(BT_L3);
    k.mov.agachar = agachar_;
    k.acenar = c.pressionou(BT_ESQUERDA) && !bloquear;
    // Combate (R1/L1 só trocam de item sozinhos: os dois juntos são os atalhos de depuração).
    k.comb.yaw = cam_.yaw_rad();
    k.comb.pitch = cam_.pitch * 3.14159265f / 180.0f;
    if (!bloquear) {
        k.comb.atirar = c.segurando(BT_R2);
        k.comb.mirar = c.segurando(BT_L2);
        k.comb.recarregar = c.pressionou(BT_QUADRADO);
        k.comb.interagir = c.segurando(BT_TRIANGULO);
        if (c.pressionou(BT_R1) && !c.segurando(BT_L1)) k.comb.ciclo = 1;
        if (c.pressionou(BT_L1) && !c.segurando(BT_R1)) k.comb.ciclo = -1;
        k.comb.picareta = c.pressionou(BT_DIREITA);
    }
    l_.passo(k, m, loot, alvos, n, tick, ev);
    cam_.atualizar(l_.p.pos, l_.p.agachado, m, dt);
}

void Jogador::montar(Cena3D& cena, const BibliotecaModelos&, const MundoColisao& m, const Loot*, f32 aspecto) {
    cam_.aplicar(cena.camera, aspecto);
    if (l_.vivo) l_.montar(cena, m, 0);
}
