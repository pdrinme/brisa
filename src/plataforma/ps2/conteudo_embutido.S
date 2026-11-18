# Pacote de conteúdo embutido no ELF (Etapa 3, BR-ADR-10). O caminho vem do Makefile.
        .section .rodata
        .balign 64
        .global brisa_pak_inicio
brisa_pak_inicio:
        .incbin BRISA_PAK_CAMINHO
        .global brisa_pak_fim
brisa_pak_fim:
        .balign 16
