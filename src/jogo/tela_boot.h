// Tela da Etapa 1: prova de vida no PS2. Fundo noturno do Ventania com rajadas
// de vento, logotipo BRISA em estilo adesivo, um quadrado girando e o estado do
// DualShock 2 na tela, com o tempo de quadro e a memória livre.
#pragma once
#include "nucleo/controle.h"
#include "nucleo/desenho2d.h"
#include "nucleo/estatisticas.h"
#include "plataforma/plataforma.h"

class TelaBoot {
public:
    // Um passo da simulação (30 Hz).
    void passo(const Controle& controle);
    void montar(ListaDesenho2D& lista, const Controle& controle, const EstatisticasQuadro& est,
                const InfoPlataforma& info) const;

    // Estado observável (testes).
    u32 passos() const { return passos_; }
    f32 angulo_graus() const { return angulo_; }
    int indice_cor() const { return cor_; }
    bool pausado() const { return pausado_; }
    f32 escala_quadrado() const { return escala_; }

    static const f32 GIRO_GRAUS_POR_SEG;  // velocidade base
private:
    u32 passos_ = 0;
    u32 vento_ = 0;          // relógio das rajadas (para na pausa)
    f32 angulo_ = 0.0f;
    f32 escala_ = 1.0f;
    int cor_ = 0;
    bool pausado_ = false;
};
