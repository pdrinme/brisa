# Capturador de tela do Brisa no emulador Play! (sem BIOS: o Play! tem BIOS em
# alto nível). Roda o ELF de verdade (EE, IOP, IRX, GS) e grava a imagem que o GS
# do emulador apresenta, renderizada por OpenGL em software (Mesa) num Xvfb.
# Ferramenta de desenvolvimento; não é distribuída com o jogo.
#   cmake -S tools/emulador/captura_play -B /opt/brisa-captura -DPLAY_SRC=/caminho/Play-
cmake_minimum_required(VERSION 3.18)
set(PLAY_SRC "" CACHE PATH "Código-fonte do Play! (com submódulos)")
set(CMAKE_MODULE_PATH ${PLAY_SRC}/deps/Dependencies/cmake-modules ${CMAKE_MODULE_PATH})
include(Header)
project(BrisaCaptura)
add_subdirectory(${PLAY_SRC}/Source ${CMAKE_CURRENT_BINARY_DIR}/Source)
add_subdirectory(${PLAY_SRC}/Source/gs/GSH_OpenGL ${CMAKE_CURRENT_BINARY_DIR}/gsh_opengl)
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
add_executable(brisa_captura main.cpp)
target_link_libraries(brisa_captura gsh_opengl PlayCore gsh_opengl PlayCore ${X11_LIBRARIES} ${OPENGL_LIBRARIES} GLX)
