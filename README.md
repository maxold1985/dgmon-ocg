# Digimon Hyper Colosseum — Starter Ver. 1 vs Starter Ver. 2

Simulador experimental em C++11 (Windows 10 / MSVC 2022), com tabuleiro Win32/GDI+, cartas `St` do **Digital Monster Card Game** de 1999 e modo automático.

## Coleções incluídas

- **Starter Ver. 1:** 60 cartas (St-1 a St-60).
- **Starter Ver. 2:** 60 cartas (St-61 a St-111 mais nove reimpressões de Starter Ver. 1).
- **Total:** 111 IDs exclusivos no `data/starter_cards.csv`; 120 entradas de coleção em `data/starter_set_index.csv`.

O banco principal `data/starter_cards.csv` agora possui **33 campos por carta**. Ele contém nomes ingleses e japoneses, categoria, espécie, atributo, campo, frame, Battle Type, ataques A/B/C, nomes de ataques revisados, classe Item/Program, valor impresso quando documentado, referência de imagem local e nível de verificação. O mesmo banco está disponível como `data/starter_cards.json`.

As páginas de coleção foram usadas para conferir os 111 IDs. **19 fichas individuais** foram consultadas para coletar propriedades adicionais; isso não significa que os 111 efeitos tenham sido implementados. As demais cartas preservam `verification_level=set_list_only`. Lost Points e requisitos de evolução ficam vazios onde ainda não foram verificados. Habilidades como `sky`, `underwater` e `underground` podem constar da ficha, mas ainda não estão implementadas no motor.

Fontes: https://wikimon.net/Starter_Ver._1 e https://wikimon.net/Starter_Ver._2 .

## Atualizar e conferir o banco de dados

O programa offline abaixo sincroniza os 111 registros com os metadados revisados, valida a composição das duas coleções e exporta o JSON. Não faz novas requisições à Wikimon.

```powershell
git pull origin mainn
.\update_starter_database.bat
```

Para somente verificar, sem modificar arquivos:

```powershell
py -3 tools\update_starter_database.py
```

Detalhes da estrutura e ressalvas de fonte: `data/README.md`. Há uma divergência documentada no tipo da carta St-64 entre as tabelas em inglês e as páginas japonesas/individual; o banco segue a página individual (`Machine`).

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


## ZIP para DAT com XOR (C++11)

O executável `hc_zip_dat.exe` empacota um ZIP em um arquivo DAT ofuscado.
Formato binário: `HCXOR01\n` (8 bytes), tamanho original `uint64` little-endian (8 bytes),
e todos os bytes ZIP com XOR de chave repetida. O algoritmo processa blocos de 64 KiB,
sem carregar o arquivo inteiro em RAM. Os bytes ZIP são preservados na decodificação.

Depois de compilar com `build_vs2022.bat`, execute no PowerShell:

```powershell
.\build_vs2022_x64\Release\hc_zip_dat.exe encode .\data\cards.zip .\data\cards.dat "minha-chave"
.\build_vs2022_x64\Release\hc_zip_dat.exe decode .\data\cards.dat .\data\cards_restauradas.zip "minha-chave"
```

Também existe `xor` para aplicar XOR bruto, sem cabeçalho:

```powershell
.\build_vs2022_x64\Release\hc_zip_dat.exe xor .\data\cards.zip .\data\cards_raw.dat "minha-chave"
.\build_vs2022_x64\Release\hc_zip_dat.exe xor .\data\cards_raw.dat .\data\cards_restauradas.zip "minha-chave"
```

Use caminhos de entrada e saída diferentes. A opção `decode` confere o cabeçalho,
o comprimento do pacote e os quatro bytes de assinatura ZIP recuperados. A assinatura
não é autenticação criptográfica nem uma validação integral do arquivo ZIP.

**Aviso:** XOR com chave repetida **não é criptografia segura**. A chave é passada
na linha de comando e pode ser visível no histórico/processos. Para proteger
conteúdo confidencial use criptografia autenticada, como AES-GCM.

O CTest `xor_zip_dat_roundtrip` valida codificação, restauração byte a byte,
XOR bruto, chave incorreta e rejeição de caminhos idênticos.


## Carregar cards.dat diretamente na RAM

O tabuleiro agora suporta `DatArchive`: abre `data/cards.dat`, desfaz XOR na RAM,
lê `starter_cards.csv` sem arquivo temporário e decodifica as imagens JPG/PNG
do ZIP para GDI+ por `IStream` em memória.

**Importante:** o leitor atual aceita apenas ZIP com método `ZIP_STORED`
(sem compressão). ZIP Deflate, ZIP64 e ZIP com senha não são aceitos. Use
`tools/pack_starter_zip.py` para criar o ZIP compatível.

```powershell
git pull origin mainn
.\build_vs2022.bat
$env:HC_DAT_KEY = "minha-chave"
.\pack_cards_dat.bat
.\build_vs2022_x64\Release\hc_card_viewer.exe
```

Com `data/cards.dat` presente e `HC_DAT_KEY` configurada, o jogo usa
o DAT e **não consulta os JPGs ou CSV no disco** durante a partida.
Sem chave ou sem DAT, mantém o carregamento antigo pelo CSV/JPG local.

Também é possível especificar o arquivo e a chave na linha de comando:

```powershell
.\build_vs2022_x64\Release\hc_card_viewer.exe --dat "C:\jogo\cards.dat" "minha-chave"
```

O DAT permanece ofuscado no disco, mas o conteúdo é decodificado em RAM;
XOR com chave repetida não oferece segurança criptográfica.
