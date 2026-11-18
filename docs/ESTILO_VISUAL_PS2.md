# Direção visual do Brisa

Base: `docs/art/GUIA_DE_ESTILO.md` do Ventania. **Referência prioritária:** a
vitrine do kit do Ventania (`docs/art/vitrine_kit_quality.png` de lá): dois
personagens Recruta, construções de madeira, pedra e metal, baús
azul-petróleo/amarelo e vegetação geométrica sobre grama saturada. A descrição
do pedido ("os dois personagens, construções e baús") bate com essa imagem.

> As imagens anunciadas "junto deste prompt" não chegaram nesta sessão. Se
> houver outras referências, elas entram aqui e podem ajustar esta página.

## O que se preserva do Ventania (sem negociação)

- **Silhueta antes do detalhe:** se a forma não se lê com 30 px de altura, simplifique. No PS2 isso vale ainda mais (640×448 numa TV).
- **Cor chapada**, a mesma paleta de 40 cores, sem texturas pintadas. O volume vem da luz.
- **Exagero controlado:** cabeça, mãos e pés grandes, cantos chanfrados.
- **Cor com função:** construções do jogador em madeira (quente) e pedra/metal (neutros); **loot azul-petróleo + amarelo**; personagem sempre com um acento forte (o cachecol coral do Recruta).
- **Tema vento:** redemoinhos, cachecóis, óculos de aviador, cata-ventos, bandeirolas.
- **Personagens do Ventania** (os mesmos 11), não genéricos.

## Como fica "de PS2" (e não um jogo moderno reduzido)

| Aspecto | Ventania (Godot) | Brisa (PS2) |
|---|---|---|
| Luz | Shader toon de 2 faixas, sombra azulada, rim | As mesmas 2 faixas e a sombra azulada, por vértice no VU1 (Gouraud); rim opcional |
| Sombras | Shadow map (perfis Médio/Alto) | **Blob** sob personagens, baús e itens; as construções escurecem o lado oposto ao sol na cor de vértice |
| Distância | Visibilidade por objeto, neblina | **Neblina do GS** na cor do céu, distância curta (~120 m) e LOD agressivo, como em jogos da época |
| Texturas | Atlas de paleta 256² | Quase nenhuma: cor de vértice. Texturas pequenas em CLUT só para fonte, HUD, céu e efeitos |
| Rachaduras nas peças | Voronoi no shader | 2–3 estágios de dano por troca de malha/decal e escurecimento da cor |
| Vidro e água (Capitão Maré) | Shaders próprios | Alfa do GS + cor de vértice; peixes só em close-up |
| Imagem final | Antialiasing e resolução nativa | 640×448 entrelaçado, com o "flicker filter" que a TV pede para a interface |
| Interface | "Adesivo ao vento" em vetor | A mesma linguagem em sprites: contorno grosso, sombra dura, botões inclinados, amarelo só na ação principal |
| Animação | Procedural + IK | As mesmas animações amostradas a 30 Hz; IK só perto da câmera |

## Orçamento de leitura em TV

- Texto do corpo com pelo menos 18 px, títulos com 28–44 px. Contorno de 2 px no texto sobre o 3D.
- Área segura: tudo o que importa dentro de 90% da tela.
- Sem linhas horizontais de 1 px (cintilam no entrelaçado).
- Cores de raridade **sempre com forma ou ícone junto** (legível em TV ruim e para daltônicos).
