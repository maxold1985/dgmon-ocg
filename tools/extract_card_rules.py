#!/usr/bin/env python3
"""Extract verifiable card-table data from locally cached Wikimon set pages.

This does not OCR card images or invent effects absent from HTML.
"""
import argparse
import csv
import json
import re
from html.parser import HTMLParser
from pathlib import Path

CARD = re.compile(r"\b(?:St|Bo|Bx|Ex|Sp|Pr|P|Ta|Da)-[0-9]+[A-Za-z]*\b", re.I)

class Tables(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.depth = 0
        self.row = None
        self.cell = None
        self.rows = []
        self.href = None
    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if tag == "table":
            self.depth += 1
        elif tag == "tr" and self.depth and self.row is None:
            self.row = []
        elif tag in ("td", "th") and self.row is not None and self.cell is None:
            self.cell = []
        elif tag == "a":
            self.href = a.get("href", "")
    def handle_data(self, data):
        if self.cell is not None:
            self.cell.append(data)
    def handle_endtag(self, tag):
        if tag in ("td", "th") and self.cell is not None:
            self.row.append(" ".join("".join(self.cell).split()))
            self.cell = None
        elif tag == "tr" and self.row is not None:
            if self.row:
                self.rows.append(self.row)
            self.row = None
        elif tag == "table":
            self.depth = max(0, self.depth - 1)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", default="data/cards_crawled.csv")
    ap.add_argument("--cache", default="cache_wikimon")
    ap.add_argument("--output", default="data/card_rules.csv")
    ap.add_argument("--json", default="data/card_rules.json")
    args = ap.parse_args()
    with open(args.csv, newline="", encoding="utf-8-sig") as f:
        catalog = list(csv.DictReader(f))
    evidence = {}
    for page in sorted(Path(args.cache).glob("*.html")):
        parser = Tables()
        parser.feed(page.read_text(encoding="utf-8", errors="replace"))
        for row in parser.rows:
            joined = " | ".join(row)
            matches = list(CARD.finditer(joined))
            if len(matches) != 1:
                continue
            card_id = matches[0].group().lower()
            if card_id not in evidence:
                evidence[card_id] = {"source_cache": str(page), "raw_table_row": joined,
                                     "source_page": "https://wikimon.net/" + page.stem}
    fields = ["id", "name", "kind", "source_cache", "source_page",
              "raw_table_row", "effect_text", "attack_a", "attack_b", "attack_c",
              "evolution_requirements", "verification_status"]
    output = []
    for card in catalog:
        key = card.get("id", "").strip().lower()
        item = {k: "" for k in fields}
        for k in ("id", "name", "kind"):
            item[k] = card.get(k, "")
        item.update(evidence.get(key, {}))
        item["verification_status"] = ("table_row_only_needs_card_rule_review"
                                       if key in evidence else "no_rule_text_found")
        output.append(item)
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, "w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(output)
    with open(args.json, "w", encoding="utf-8") as f:
        json.dump(output, f, ensure_ascii=False, indent=2)
    print("Catalog cards:", len(output))
    print("Cached table evidence:", len(evidence))
    print("CSV:", args.output, "JSON:", args.json)
    print("Note: effects, attacks and evolution requirements are left blank until verified.")

if __name__ == "__main__":
    main()
