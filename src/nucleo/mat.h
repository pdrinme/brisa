// Matemática 3D do núcleo (portátil). Matrizes 4×4 em coluna principal
// (m[coluna*4 + linha]), vetores coluna: v' = M · v. Sistema da câmera como no
// OpenGL: y para cima, câmera olhando para −z, recorte −w ≤ x,y,z ≤ w.
#pragma once
#include "tipos.h"

struct Vec3 {
    f32 x = 0, y = 0, z = 0;
    Vec3() = default;
    constexpr Vec3(f32 a, f32 b, f32 c) : x(a), y(b), z(c) {}
};
struct Vec4 {
    f32 x = 0, y = 0, z = 0, w = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, f32 s) { return {a.x * s, a.y * s, a.z * s}; }
inline f32 ponto(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 vetorial(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
f32 comprimento(Vec3 a);
Vec3 normalizar(Vec3 a);

struct alignas(16) Mat4 {
    f32 m[16];
    static Mat4 identidade();
    static Mat4 translacao(Vec3 t);
    static Mat4 escala(f32 s);
    static Mat4 rotacao_y(f32 rad);
    static Mat4 rotacao_x(f32 rad);
    static Mat4 rotacao_z(f32 rad);
    // Translação · rotação (quaternion x,y,z,w) · escala, como os nós do glTF.
    static Mat4 trs(Vec3 t, const f32 q[4], Vec3 s);
    // fovy em radianos; aspecto = largura/altura da imagem NA TELA (4:3 numa TV).
    static Mat4 perspectiva(f32 fovy, f32 aspecto, f32 perto, f32 longe);
    static Mat4 olhar_para(Vec3 olho, Vec3 alvo, Vec3 cima);
    f32& operator()(int linha, int coluna) { return m[coluna * 4 + linha]; }
    f32 operator()(int linha, int coluna) const { return m[coluna * 4 + linha]; }
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 transformar(const Mat4& m, Vec3 p);        // w do ponto = 1
Vec3 transformar_direcao(const Mat4& m, Vec3 d);  // ignora a translação
// Inversa de uma matriz rígida com escala uniforme (rotação · escala · translação).
Mat4 inversa_rigida(const Mat4& m);

// Frustum: 6 planos (a, b, c, d) com a normal para dentro, extraídos de P·V.
struct Frustum {
    Vec4 planos[6];  // esquerda, direita, baixo, cima, perto, longe
    static Frustum de(const Mat4& vp);
    // 0 = fora, 1 = dentro, 2 = cruza algum plano. cruza_perto informa se cruza o plano perto.
    int esfera(Vec3 c, f32 r, bool* cruza_perto = nullptr) const;
};
