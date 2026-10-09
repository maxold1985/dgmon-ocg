# Digimon Hyper Colosseum OCG (1999–2002)

Protótipo C++11 do motor de regras e catálogo das cartas clássicas japonesas.

## Código-fonte

O arquivo `digimon_hyper_colosseum_2002_cards_integrated.zip` contém o projeto completo da versão inicial (arquivos `src/`, `include/`, `data/`, `tests/`, `tools/`, `CMakeLists.txt` e scripts .bat).

Para compilar depois de extrair:

```bat
cmake -S . -B build -G "Visual Studio 12 2013"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

**Estado:** catálogo offline de 283 IDs de cartas; somente uma parte possui os atributos/evoluções necessários para batalhas. O motor ainda não implementa todos os efeitos oficiais de 2002. O renderizador DirectX 11 ainda não está presente. Não inclui imagens de cartas.
