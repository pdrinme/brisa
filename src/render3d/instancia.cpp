#include "instancia.h"

int adicionar_modelo(Cena3D& cena, const ModeloBrisa& m, const Pose* pose, const Mat4& inst) {
    static Pose repouso;
    if (!pose) {
        pose_repouso(m, repouso);
        pose = &repouso;
    }
    int n = 0;
    for (int i = 0; i < m.n_partes(); i++) {
        int no = m.no_da_parte(i);
        Mat4 mdl = inst;
        if (no >= 0) {
            Mat4 inv;
            for (int k = 0; k < 16; k++) inv.m[k] = m.no(no).inv_bind[k];
            mdl = inst * pose->mundo[no] * inv;
        }
        if (cena.adicionar(&m.malha(i), mdl)) n++;
    }
    return n;
}
