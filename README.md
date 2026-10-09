# Digimon Hyper Colosseum OCG (1999–2002)

Motor experimental de cartas em C++11 com catálogo offline de 283 registros. Ainda não contém interface DirectX 11.

## Visual Studio 2022 (MSVC v143)

Instale **Desktop development with C++**, o toolset **MSVC v143**, Windows SDK e CMake.

```bat
git clone -b mainn https://github.com/maxold1985/dgmon-ocg.git
cd dgmon-ocg
build_vs2022.bat
```

O script gera a solução x64, compila Release, executa CTest e inicia a demonstração. Para Win32:

```bat
build_vs2022.bat Win32
```

Comandos manuais:

```bat
cmake -S . -B build_vs2022_x64 -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build_vs2022_x64 --config Release
ctest --test-dir build_vs2022_x64 -C Release --output-on-failure
```

Abra `build_vs2022_x64/digimon_hyper_colosseum_2002.sln` no Visual Studio. O projeto inicial é `hc_cards`.

O catálogo comprimido em `data/cards.tar.gz` é extraído para o diretório de build, sem alterar os fontes. Os arquivos `include/`, `src/` e `tests/` permanecem em C++11.

## Limitações

O motor implementa preparação, evolução básica, batalha e pontos, mas ainda não cobre todos os efeitos oficiais, Option Cards, Jogress ou o renderizador DX11. Alguns registros do catálogo não são utilizáveis em batalhas. O script legado `sync_1999_2002.bat` depende de um importador ainda não publicado e não é necessário para o build offline.

**Nota:** a configuração MSVC 2022 foi publicada; o build ainda não foi validado em um computador Windows com Visual Studio 2022.
