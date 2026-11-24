// Etapa 8: roteiro de uma partida BR jogada pelo autoteste (igual no PC e no PS2,
// e usado também por tests/teste_partida.cpp). O roteiro olha o estado da
// partida e aperta os botões como um jogador:
//   contagem → no ônibus, ✕ quando as portas abrem (+1,5 s) → queda olhando para
//   baixo (mergulho), depois para a frente → planador → pousa → 3 s depois de a
//   Tormenta nascer (de frente para a parede), ela o elimina (atalho do roteiro) → assiste, ✕ troca → resultado.
// Cada passo registra um marco BRISA_PARTIDA no log.
#pragma once
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ilha.h"
#include "nucleo/controle.h"
#include "plataforma/plataforma.h"

class RoteiroPartida {
public:
    enum Marco { M_CONTAGEM = 1, M_ONIBUS = 2, M_SALTOU = 4, M_MERGULHO = 8, M_PLANADOR = 16, M_POUSOU = 32, M_ELIMINADO = 64,
                 M_ASSISTINDO = 128, M_TROCOU = 256, M_RESULTADO = 512, M_TORMENTA = 1024, TODOS = 2047 };
    u32 marcos = 0;
    void comecar(Ilha& il, u32 semente) {
        il.comecar_partida(semente, true);
        fase_ = 0;
        t_ = 0;
        abertas_ = 0;
        marcos = 0;
        marcar(il, M_CONTAGEM, "contagem");
    }
    bool terminou() const { return fase_ >= 6; }
    // Preenche o controle deste passo (antes de Ilha::passo).
    void controle(Ilha& il, EstadoControle& e) {
        e = EstadoControle();
        e.modo = ModoControle::ANALOGICO;
        const PartidaBR& p = il.partida();
        const Lutador& j = il.jogador().lutador();
        t_++;
        switch (fase_) {
        case 0:  // contagem e ônibus
            if (p.estado() == PART_ONIBUS && !(marcos & M_ONIBUS)) { marcar(il, M_ONIBUS, "onibus"); t_ = 0; }
            if (p.estado() == PART_ONIBUS && p.onibus().portas_abertas(il.passos() + 1)) {
                if (++abertas_ >= 45 && (abertas_ & 1)) e.botoes = BT_XIS;  // salta 1,5 s depois de abrir
            }
            if (j.p.queda == QUEDA_LIVRE) { marcar(il, M_SALTOU, "saltou"); fase_ = 1; t_ = 0; }
            break;
        case 1:  // queda: para a frente, olhando para baixo (mergulho), depois de volta
            e.ly = 0;
            if (t_ < 40) e.ry = 255;
            else if (t_ >= 90 && t_ < 106) e.ry = 0;  // volta a olhar para a frente (~70°)
            if (!(marcos & M_MERGULHO) && !strcmp(il.jogador().animador().estado(), "mergulho")) marcar(il, M_MERGULHO, "mergulho");
            if (j.p.queda == PLANADOR && !(marcos & M_PLANADOR)) marcar(il, M_PLANADOR, "planador");
            if (j.p.queda == NO_CHAO_NORMAL && j.p.no_chao) { marcar(il, M_POUSOU, "pousou"); fase_ = 2; t_ = 0; }
            break;
        case 2:  // no chão; 3 s depois de a Tormenta nascer, ela o elimina (atalho do roteiro)
            if (p.estado() != PART_JOGO) t_ = 0;
            if (p.estado() == PART_JOGO && !(marcos & M_TORMENTA)) {
                // vira de frente para a parede da Tormenta mais perto (captura do autoteste)
                const Tormenta& tm = p.tormenta();
                f32 dx = j.p.pos.x - tm.cx(), dz = j.p.pos.z - tm.cz();
                il.jogador_mut().reposicionar(j.p.pos, atan2f(-dx, -dz) * 57.29578f, 4.0f);
                marcar(il, M_TORMENTA, "tormenta");
            }
            if (t_ >= 90) {
                il.bots().eliminar_pela_tormenta(il.jogador_mut().lutador(), il.passos());
                marcar(il, M_ELIMINADO, "eliminado");
                fase_ = 3;
                t_ = 0;
            }
            break;
        case 3:  // assistindo; ✕ troca de alvo
            if (il.jogador().espectado() && !(marcos & M_ASSISTINDO)) marcar(il, M_ASSISTINDO, "assistindo");
            if (t_ == 30) e.botoes = BT_XIS;
            if (t_ == 32) { marcar(il, M_TROCOU, "trocou"); fase_ = 4; t_ = 0; }
            break;
        case 4:  // a tela de resultado aparece 2 s depois da eliminação
            if (il.resultado_visivel()) { marcar(il, M_RESULTADO, "resultado"); fase_ = 5; t_ = 0; }
            break;
        case 5:
            if (t_ >= 45) fase_ = 6;
            break;
        default: break;
        }
    }
private:
    void marcar(Ilha& il, u32 m, const char* nome) {
        marcos |= m;
        const PartidaBR& p = il.partida();
        const Lutador& j = il.jogador().lutador();
        const Lutador* a = il.jogador().espectado();
        ResultadoPartida r = p.resultado(Jogador::ID);
        char msg[320];
        snprintf(msg, sizeof msg,
                 "BRISA_PARTIDA %s passo=%u estado=%d vivos=%d/%d queda=%d pos=%.1f,%.1f,%.1f vel_y=%.1f anim=%s vivo=%d assistindo=%s "
                 "colocacao=%d tempo_s=%d portas=%u-%u\n",
                 nome, static_cast<unsigned>(il.passos()), p.estado(), p.vivos(), p.total(), j.p.queda, static_cast<double>(j.p.pos.x),
                 static_cast<double>(j.p.pos.y), static_cast<double>(j.p.pos.z), static_cast<double>(j.p.vel.y), il.jogador().animador().estado(),
                 j.vivo ? 1 : 0, a ? a->nome : "-", r.colocacao, r.tempo_s, static_cast<unsigned>(p.onibus().abre),
                 static_cast<unsigned>(p.onibus().fecha));
        plataforma::log(msg);
    }
    int fase_ = 0, abertas_ = 0;
    u32 t_ = 0;
};
