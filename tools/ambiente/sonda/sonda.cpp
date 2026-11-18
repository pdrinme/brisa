// Sonda do toolchain (Etapa 0): só confere compilação e ligação. Não é código do jogo
// e não é executada (não inicializa vídeo nem carrega IRX).
#include <kernel.h>
#include <libpad.h>
#include <libmc.h>
#include <audsrv.h>
#include <sifrpc.h>
#include <cstdint>
#include <cstring>
struct V { float x, y, z; };
template <typename T, int N> struct Pool { T v[N]; int n = 0; };
static Pool<V, 64> pool;
int main() {
  SifInitRpc(0);
  padInit(0);
  mcInit(MC_TYPE_MC);
  audsrv_init();
  pool.v[pool.n++] = {1, 2, 3};
  return (int)pool.v[0].x;
}
