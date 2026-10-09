# Digimon Hyper Colosseum OCG (1999–2002)

Motor experimental Hyper Colosseum em C++11. O CardCatalog carrega apenas IDs Bo-1 a Bo-300 do catálogo offline; não inventa regras individuais. Inclui tabuleiro Win32/GDI+ inspirado no WonderSwan Color (não é DirectX 11).

## Visual Studio 2022 (MSVC v143)

Instale **Desktop development with C++**, o toolset **MSVC v143**, Windows SDK e CMake.

```bat
git clone -b mainn https://github.com/maxold1985/dgmon-ocg.git
cd dgmon-ocg
build_vs2022.bat
```

O script gera a solução x64, compila Release, executa CTest e abre o tabuleiro gráfico (hc_card_viewer.exe). Para Win32:

```bat
build_vs2022.bat Win32
```

Comandos manuais:

```bat
cmake -S . -B build_vs2022_x64 -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build_vs2022_x64 --config Release
ctest --test-dir build_vs2022_x64 -C Release --output-on-failure
```

Abra `build_vs2022_x64/digimon_hyper_colosseum_2002.sln` no Visual Studio. O projeto inicial é `hc_card_viewer` no Windows.

O catálogo comprimido em `data/cards.tar.gz` é extraído para o diretório de build, sem alterar os fontes. Os arquivos `include/`, `src/` e `tests/` permanecem em C++11.

## Tabuleiro gráfico (Windows)

Depois de compilar:

```bat
build_vs2022_x64\Release\hc_card_viewer.exe
```

O tabuleiro exibe os campos do adversário e do jogador, Digimon ativo, suporte, requisitos e custo de evolução, medidor de pontos, Net Ocean, Dark Area, mão com até seis cartas e prévia grande da carta selecionada.

**Controles:** o modo AUTO inicia ao abrir a janela. A (ou Espaço) pausa/retoma; +/- muda a velocidade (0,55 / 1,1 / 2,2 segundos por passo). N inicia uma partida manual nova; setas esquerda/direita navegam o catálogo; clique na mão para selecionar cartas; R repõe a mão; Enter conclui a preparação; Esc fecha a janela. Os botões laterais oferecem Nova, Auto, velocidade, Descartar, Planejar evolução, Evoluir, Batalha e Pontos.

Os JPGs são carregados da pasta `data/card_images_original/Bo-1.jpg`, etc., na **raiz do repositório**; há fallback para `data/card_images/` com miniaturas. Os caminhos são definidos na configuração do CMake, portanto não é necessário copiar imagens para Release. Se as imagens ainda não foram baixadas, o tabuleiro mostra o número da carta e "SEM JPG".

**Jogo automático:** o controlador em `src/auto_player.cpp` avança cada fase em um temporizador Win32: repõe cartas, tenta uma evolução válida, conclui a preparação de ambos os jogadores, evolui, resolve a batalha e os pontos; a partida encerra segundo o motor. Se não houver cartas Bo suficientes com regras verificadas para construir dois decks de 30, ativa **DEMO VISUAL**: percorre as imagens das cartas automaticamente, sem atribuir vitórias, ataques ou pontos fictícios. O status e cabeçalho identificam explicitamente o modo.

O modo prévia exibe cartas do catálogo mesmo sem dados suficientes para jogar. Uma partida só é iniciada quando existirem **10 nomes distintos de Digimon verificados, incluindo um Level III**, permitindo 30 cartas no baralho (3 cópias por nome). O adversário passa nas fases de evolução e preparação; habilidades especiais não verificadas, suportes e Option Cards ainda não são jogáveis. Não é uma emulação exata do WonderSwan.

## Limitações

O motor implementa preparação, evolução básica, batalha e pontos, mas ainda não cobre todos os efeitos oficiais, Option Cards, Jogress ou o renderizador DX11. Alguns registros do catálogo não são utilizáveis em batalhas. O script legado `sync_1999_2002.bat` depende de um importador ainda não publicado e não é necessário para o build offline.

**Nota:** a configuração MSVC 2022 foi publicada; o build ainda não foi validado em um computador Windows com Visual Studio 2022.
