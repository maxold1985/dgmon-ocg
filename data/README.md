# Hyper Colosseum Starter card database

This folder contains **111 unique card IDs** from **120 printed set entries** in
Digital Monster Card Game Starter Ver. 1 and Starter Ver. 2 (1999).

## Files

- `starter_cards.csv`: canonical **33-column** catalog loaded by C++.
- `starter_card_metadata.csv`: independently maintained 17-column enrichment source.
- `starter_set_index.csv`: 120 card/set membership records (60 per set), including reprints.
- `starter_cards.json`: optional generated export; use `update_starter_database.bat`.

## Data provenance

Set inventory, printed Digimon names (English/Japanese), frame, types, attributes,
fields, battle types and attack values are based on:

- https://wikimon.net/Starter_Ver._1 (June 1999, 60 entries)
- https://wikimon.net/Starter_Ver._2 (November 1999, 60 entries)

Individual card properties for **St-1, St-2, St-3, St-5, St-7, St-9, St-11,
St-13, St-18, St-23, St-24, St-62, St-63, St-64, St-65 and St-111**
were checked against their respective `https://wikimon.net/St-N` pages.

For **St-49, St-50 and St-51**, the individual Wikimon pages verify attack
selection (A/C/B), Battle Phase timing, discard at end of turn and printed values
(+30/+50/+30). The numeric printed values are **not implemented as combat
bonuses**: their gameplay semantics have not been established in the current engine.

**St-64 conflict:** the English Starter Ver. 2 list calls the species
`Mutant`, but the Japanese list says `マシーン型`, and the individual
St-64 page gives `Machine`. The metadata uses `Machine` and records the
source discrepancy instead of silently overwriting it.

## Evidence status

- `verification_level=individual_page_reviewed`: individual Wikimon page
  consulted; selected attacks, classifications and/or Item descriptions recorded.
  This does not imply every rule has been implemented.
- `verification_level=set_list_only`: only the collection table was used.
  Attack names, Lost Points, card-specific evolution costs and special effects
  remain empty/unknown unless independently confirmed.
- `effect_status` is the separate simulation compatibility flag, and does not
  replace evidence status. A `passive_only` ability such as `sky` remains
  unimplemented where the engine does not support its actual effect.
- `image_file` is the **expected local filename**, not proof that an image
  exists. Download card artwork separately; observe individual image licenses.

To verify and synchronize from **local reviewed metadata** (no network requests):

```powershell
.\update_starter_database.bat
```

The Python script checks 111 unique cards, 60 entries per starter,
33-column schema, source metadata and image filename conventions. It
exports `data/starter_cards.json`.

To fetch existing original card images with deliberate pacing, run
`download_starter_images.bat`. Image bytes are not included here.

Wikimon text/metadata is attributed to Wikimon and its contributors;
consult [Wikimon's general disclaimer](https://wikimon.net/Wikimon:General_disclaimer)
and its current site terms before redistribution, and review each image's own licensing.
