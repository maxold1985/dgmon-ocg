#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fetch the two official starter gallery pages, then original card art.

Images are never redrawn/reconstructed: only original Wikimon file bytes.
"""
import argparse
import random
import sys
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

import download_card_images_original

SETS = (
    ("Starter Ver. 1", "https://wikimon.net/Starter_Ver._1", "starter_set_1.html"),
    ("Starter Ver. 2", "https://wikimon.net/Starter_Ver._2", "starter_set_2.html"),
)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0,
                        help="Maximum new originals (0 = all)")
    parser.add_argument("--delay", type=float, default=4.0)
    parser.add_argument("--cache", default="cache_wikimon")
    parser.add_argument("--output", default="data/card_images_original")
    args = parser.parse_args()
    if args.delay < 3 or args.limit < 0:
        parser.error("--delay >= 3 and --limit >= 0 required")
    cache = Path(args.cache)
    cache.mkdir(parents=True, exist_ok=True)
    had_request = False
    for label, url, filename in SETS:
        target = cache / filename
        if target.exists() and target.stat().st_size:
            print("Set page cached:", label, flush=True)
            continue
        if had_request:
            time.sleep(args.delay + random.uniform(0, 1.5))
        print("Fetching set gallery:", label, flush=True)
        request = Request(url, headers={
            "User-Agent": "DigimonOCGResearch/1.0 (noncommercial catalog study)"
        })
        try:
            with urlopen(request, timeout=25) as response:
                content_type = response.headers.get("Content-Type", "").lower()
                if "text/html" not in content_type:
                    raise RuntimeError("Expected HTML: " + content_type)
                data = response.read(3 * 1024 * 1024 + 1)
            had_request = True
            if len(data) > 3 * 1024 * 1024:
                raise RuntimeError("Page exceeds 3 MiB")
            temporary = target.with_suffix(".html.tmp")
            temporary.write_bytes(data)
            temporary.replace(target)
        except (HTTPError, URLError, OSError, RuntimeError) as exc:
            print("Unable to obtain %s: %s" % (label, exc), flush=True)
            return 1
    if had_request:
        time.sleep(args.delay + random.uniform(0, 1.5))
    print("Downloading original images for St-1 to St-111 (111 unique IDs)", flush=True)
    sys.argv = [
        "download_card_images_original.py",
        "--csv", "data/starter_cards.csv",
        "--cache", str(cache),
        "--output", args.output,
        "--delay", str(args.delay),
        "--limit", str(args.limit),
    ]
    download_card_images_original.main()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
