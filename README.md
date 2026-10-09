# Digimon Hyper Colosseum OCG (1999–2002)

Protótipo do card game clássico japonês, com **motor de regras em C++11** e catálogo de cartas; projetado para futura interface DirectX 11.

## Conteúdo

- `include/engine.h` e `src/engine.cpp`: preparação, evolução básica, combate A/B/C e contagem de pontos.
- `include/card_catalog.h`, `src/card_catalog.cpp`: leitura de CSV, busca por coleção/número, verificação de campos e ponte para o motor.
- `src/main.cpp`: aplicativo de terminal para consultar e testar as cartas.
- `tests/`: testes C++ do motor e protótipos de testes de importador.
- `data/cards.tar.gz`: catálogo offline com **283 registros** do Starter Ver. 1, Starter Ver. 2, Starter Ver. 7, Booster 1 e Booster 15. O CMake extrai automaticamente `data/cards.csv` durante a configuração.
- `build_vs2013.bat`: compilação no Visual Studio 2013 / Win32.

## Compilação — Visual Studio 2013

Necessário: CMake compatível e compilador de C++ do Visual Studio 2013.

```bat
cmake -S . -B build -G "Visual Studio 12 2013"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build\Release\hc_cards.exe data\cards.csv --demo
```

Alternativamente, execute `build_vs2013.bat`.

## Estado e limitações

**Protótipo incompleto, não é ainda uma implementação fiel de todas as cartas/regras japonesas.** Muitas cartas estão cadastradas somente para consulta, sem atributos ou efeitos prontos para uso em batalha; Winning Percentage, Jogress, Option Cards e efeitos específicos ainda precisam ser concluídos. Não inclui renderizador DirectX 11 nem imagens de cartas.

O CSV tem campos incompletos intencionalmente. A ponte desabilita cartas sem os dados essenciais; **não inventar** valores ausentes.

O script `sync_1999_2002.bat` é uma entrada para o futuro importador Wikimon. Para executá-lo é necessário adicionar `tools/sync_wikimon.py` ao projeto (disponível no arquivo ZIP original gerado na conversa). O catálogo offline já está incluído no repositório; não requer o sincronizador para a compilação.
