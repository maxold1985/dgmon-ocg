# Digimon Hyper Colosseum — Starter Ver. 1 vs Starter Ver. 2

Simulador experimental em C++11 (Windows 10 / MSVC 2022), com tabuleiro Win32/GDI+, cartas `St` do **Digital Monster Card Game** de 1999 e modo automático.

## Coleções incluídas

- **Starter Ver. 1:** 60 cartas (St-1 a St-60).
- **Starter Ver. 2:** 60 cartas (St-61 a St-111 mais nove reimpressões de Starter Ver. 1).
- **Total:** 111 IDs exclusivos no `data/starter_cards.csv`; 120 entradas de coleção em `data/starter_set_index.csv`.

Os nomes, Battle Type, níveis e ataques A/B/C foram transcritos das tabelas dos dois starters da Wikimon. Os campos de Lost Points e requisitos/evoluções são preenchidos **somente para as cartas consultadas individualmente**. As demais cartas têm `effect_status=unverified` e não podem ser jogadas enquanto faltarem regras verificadas.

Fontes: https://wikimon.net/Starter_Ver._1 e https://wikimon.net/Starter_Ver._2 .

## Compilar e abrir

Instale Visual Studio 2022, MSVC v143, Windows SDK e CMake.

```powershell
git pull origin mainn
.\build_vs2022.bat
```

O script compila, executa CTest e abre `build_vs2022_x64\Release\hc_card_viewer.exe`. No Visual Studio, o projeto inicial é `hc_card_viewer`.

## Imagens originais das cartas

As imagens não são incluídas no repositório. Para baixá-las da Wikimon, com requisições sequenciais, cache, intervalo mínimo de 4 segundos, e retomada:

```powershell
.\download_starter_images.bat --limit 10
.\download_starter_images.bat
```

O script consulta as páginas oficiais dos dois starters e extrai os arquivos originais das galerias (sem miniaturas 60px). Os JPGs ficam em `data\card_images_original\St-1.jpg`, etc. Se a carta já existir, o download é ignorado. O tabuleiro também tenta encontrar miniaturas locais em `data\card_images` como último recurso.

Antes de redistribuir imagens, confira a licença específica de cada arquivo na Wikimon.

## Jogo automático e tabuleiro

- **ST1 (embaixo):** jogador automático ou manual, inicia com St-1 Agumon.
- **ST2 (em cima):** adversário automático, inicia com St-62 Gottsumon.
- Cada lado recebe um baralho experimental de **30 cartas** construído com nove Digimon de nomes distintos (3 cópias cada) e três cópias do item St-49.
- O tabuleiro exibe cartas ativas, mão, deck/Net Ocean, Dark Area, pontos, fases, imagens, nomes e ataques.
- O motor executa as fases Preparação, Evolução, Batalha e Pontos; não gera ataques aleatórios nem resultados inventados.
- **A ou Espaço:** automático/pausar; **+/-:** velocidade; **N:** nova partida manual; **setas:** navegar no catálogo; **clique direito:** detalhes da carta, Lost Points, evolução e fonte; **Esc:** sair.
- Na Batalha manual, selecione um item na mão e clique **USAR ITEM**. O adversário só toma decisões automaticamente quando o modo Auto está ativo.

A automação inicia ao abrir a janela. Se os decks não puderem ser formados com regras verificadas, o tabuleiro indica explicitamente que está em modo visual (sem simular combate).

### Regras implementadas nesta etapa

- Níveis III / IV, poderes A/B/C e Lost Points das cartas previamente verificadas.
- Evolução St-2 (Greymon) a partir de Agumon com custo **OO** (duas cartas do Net Ocean), conforme a página de St-2.
- Plug-In **St-49**: seleciona ataque A; **St-50**: seleciona ataque C; **St-51**: seleciona ataque B, independentemente do Battle Type adversário; o item vai para Dark Area ao final do turno.
- Ataque C de guarda cancela o ataque A quando a carta possui `cancel_target=A`.
- Regras genéricas do motor para decks de 30 cartas, máximo de três cópias por nome, evolução básica, seleção de batalha e pontos.

**Ainda não implementado:** os símbolos +30/+50 exibidos nos Plug-Ins, habilidades passivas especiais de campo, todas as cartas Program, Winning Percentage, Jogress, habilidades específicas e condições de evolução de outras cartas, regras completas de Option Cards, IA estratégica. Um cartão com `unverified` fica visível no catálogo mas não entra na partida.

Os CSVs originais de crawler e `data/cards.tar.gz` foram mantidos no projeto.

## Console e testes

```powershell
.\build_vs2022_x64\Release\hc_cards.exe data\starter_cards.csv --card St-2
.\build_vs2022_x64\Release\hc_cards.exe data\starter_cards.csv --starter 1
.\build_vs2022_x64\Release\hc_cards.exe data\starter_cards.csv --starter 2
.\build_vs2022_x64\Release\hc_cards.exe data\starter_cards.csv --demo
ctest --test-dir build_vs2022_x64 -C Release --output-on-failure
```

O CTest inclui validações de inventário, construção dos dois decks, partida automática e efeitos de troca de ataque. A compilação do MSVC precisa ser confirmada no computador Windows.
