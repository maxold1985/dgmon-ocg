#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Lightweight Wikimon Hyper Colosseum catalog crawler. Python 3 stdlib only."""
import argparse
import csv
import html
import os
import random
import re
import sys
import time
from html.parser import HTMLParser
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import Request, urlopen

ROOT = "https://wikimon.net/"
ID_PATTERN = re.compile(r"^(?:St|Bo|Bx|Ex|Sp|Pr|P|Ta|Da)-[0-9]+[A-Za-z]*$", re.I)
FIELDS = ["id", "set", "name_en", "name_jp", "kind", "level",
          "battle_type", "attack_a", "attack_b", "attack_c",
          "cancel_target", "lost_iii", "lost_iv", "lost_perfect",
          "lost_ultimate", "evolution_requirements", "effect_status", "source"]

class Tables(HTMLParser):
    """Collect leaf table rows, including MediaWiki's nested tables."""
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.depth = 0
        self.rows = []
        self.rowstack = []
        self.cell = None

    def handle_starttag(self, tag, attrs):
        if tag == "table":
            self.depth += 1
        elif tag == "tr" and self.depth:
            self.rowstack.append((self.depth, []))
        elif tag in ("th", "td") and self.rowstack:
            if self.rowstack[-1][0] == self.depth and self.cell is None:
                self.cell = []
        elif tag == "br" and self.cell is not None:
            self.cell.append(" ")

    def handle_data(self, data):
        if self.cell is not None:
            self.cell.append(data)

    def handle_endtag(self, tag):
        if tag in ("th", "td") and self.cell is not None and self.rowstack:
            self.rowstack[-1][1].append(" ".join("".join(self.cell).split()))
            self.cell = None
        elif tag == "tr" and self.rowstack:
            depth, row = self.rowstack.pop()
            if row:
                self.rows.append(row)
        elif tag == "table" and self.depth:
            self.depth -= 1


def fetch(url, cache, delay, timeout):
    cache.parent.mkdir(parents=True, exist_ok=True)
    if cache.exists() and cache.stat().st_size > 0:
        return cache.read_text(encoding="utf-8")
    if not fetch.robots_allowed:
        raise RuntimeError("Check the website robots.txt and permission before collecting.")
    if fetch.last_request:
        wait = max(0.0, delay + random.uniform(0, 1.5) - (time.monotonic() - fetch.last_request))
        time.sleep(wait)
    request = Request(url, headers={"User-Agent": "DigimonOCGResearch/1.0 (noncommercial catalog study)"})
    try:
        with urlopen(request, timeout=timeout) as response:
            body = response.read(8 * 1024 * 1024 + 1)
            if len(body) > 8 * 1024 * 1024:
                raise RuntimeError("Page exceeds 8 MiB safety limit")
            charset = response.headers.get_content_charset() or "utf-8"
        result = body.decode(charset, errors="replace")
        temp = cache.with_suffix(cache.suffix + ".tmp")
        temp.write_text(result, encoding="utf-8")
        temp.replace(cache)
        return result
    except HTTPError as exc:
        if exc.code in (429, 503):
            raise RuntimeError("Rate limit/server busy (HTTP %s). Stop and retry later." % exc.code)
        raise
    finally:
        fetch.last_request = time.monotonic()
fetch.last_request = None
fetch.robots_allowed = True

def parse_set(page, set_name, source):
    parser = Tables()
    parser.feed(page)
    records = {}
    # Set pages have: Card Number | Card Name | Japanese Name
    # Option pages additionally have: Type (Item or Program).
    # Combat statistics belong to individual card pages, not set lists.
    for row in parser.rows:
        if len(row) < 3:
            continue
        number = row[0].strip()
        if not ID_PATTERN.fullmatch(number):
            continue
        if number in records:
            continue
        record = dict.fromkeys(FIELDS, "")
        record.update(id=number, set=set_name, name_en=row[1],
                      name_jp=row[2],
                      kind="Option" if len(row) >= 4 and row[3] in
                      ("Item", "Program", "Option") else "Digimon",
                      effect_status="unverified", source=source)
        records[number] = record
    return records


def write_csv(records, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    temp = output.with_suffix(".tmp")
    with temp.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDS)
        writer.writeheader()
        for number in sorted(records):
            writer.writerow(records[number])
    temp.replace(output)

def main():
    p = argparse.ArgumentParser(description="Single-threaded Wikimon card index collector")
    p.add_argument("--target", type=int, default=3000)
    p.add_argument("--delay", type=float, default=4.0)
    p.add_argument("--timeout", type=int, default=15)
    p.add_argument("--cache", default="cache_wikimon")
    p.add_argument("--output", default="data/cards_crawled.csv")
    p.add_argument("--year-cutoff", type=int, default=2002,
                   help="0 = all eras, 2002 = 1999-2002 Japanese releases")
    args = p.parse_args()
    if args.target < 1 or args.delay < 3 or args.timeout < 1:
        p.error("target >= 1, delay >= 3 seconds, timeout >= 1 required")
    cache = Path(args.cache)
    output = Path(args.output)
    # The full index has many card game eras. This list is explicitly
    # limited to known Hyper Colosseum set pages.
    pages = [("Starter Ver. %d" % i, "Starter_Ver._%d" % i) for i in range(1, 9)]
    pages += [("Booster %d" % i, "Booster_%d" % i) for i in range(1, 19)]
    pages += [("Expansion Board %d" % i, "Expansion_Board_%d" % i) for i in range(2, 6)]
    pages.append(("Expansion Board", "Expansion_Board"))
    if args.year_cutoff == 0:
        print("WARNING: all-era mode is not yet enumerated; using 1999-2002 set list.")
    records = {}
    for label, slug in pages:
        url = ROOT + quote(slug, safe="._")
        path = cache / (slug.replace(".", "_") + ".html")
        try:
            page = fetch(url, path, args.delay, args.timeout)
            found = parse_set(page, label, url)
            if not found:\n                print("WARNING: 0 IDs parsed from %s; inspect cached HTML: %s" % (url, path), flush=True)\n            for key, value in found.items():
                records.setdefault(key, value)
            write_csv(records, output)
            print("%-24s %4d in set; %4d unique total" % (label, len(found), len(records)), flush=True)
            if len(records) >= args.target:
                break
        except (HTTPError, URLError, RuntimeError) as exc:
            print("STOP at %s: %s" % (label, exc), file=sys.stderr)
            break
    print("Saved %d unique IDs to %s" % (len(records), output))
    if len(records) < args.target:
        print("Target %d not reached; no invented entries added." % args.target)
    return 0

if __name__ == "__main__":
    sys.exit(main())
