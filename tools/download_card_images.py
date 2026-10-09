#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Download card gallery thumbnails from previously cached Wikimon set pages."""
import argparse
import csv
import random
import re
import time
from html.parser import HTMLParser
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urljoin
from urllib.request import Request, urlopen

ROOT = "https://wikimon.net"
CARD_IMG = re.compile(r"^((?:St|Bo|Bx|Ex|Sp|Pr|P|Ta|Da)-[0-9]+[A-Za-z]*)\.(jpg|jpeg|png|webp)$", re.I)

class GalleryImages(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.images = {}

    def handle_starttag(self, tag, attrs):
        if tag != "img":
            return
        props = dict(attrs)
        alt = props.get("alt", "")
        match = CARD_IMG.fullmatch(alt)
        src = props.get("src", "")
        if match and src.startswith(("/images/", "https://wikimon.net/images/")):
            self.images.setdefault(match.group(1).lower(), (match.group(1), match.group(2).lower(), urljoin(ROOT, src)))

def main():
    ap = argparse.ArgumentParser(description="Lightweight cached gallery image downloader")
    ap.add_argument("--csv", default="data/cards_crawled.csv")
    ap.add_argument("--cache", default="cache_wikimon")
    ap.add_argument("--output", default="data/card_images")
    ap.add_argument("--delay", type=float, default=4.0)
    ap.add_argument("--timeout", type=int, default=15)
    ap.add_argument("--limit", type=int, default=0, help="Max new downloads (0 = all)")
    args = ap.parse_args()
    if args.delay < 3 or args.timeout < 1 or args.limit < 0:
        ap.error("delay >= 3, timeout >= 1 and limit >= 0 required")
    csv_path = Path(args.csv)
    if not csv_path.exists():
        ap.error("CSV not found: %s. Run crawler_3000.bat first." % csv_path)
    with csv_path.open(encoding="utf-8-sig", newline="") as f:
        ids = {row["id"].lower() for row in csv.DictReader(f)}
    images = {}
    html_files = sorted(Path(args.cache).glob("*.html"))
    for file in html_files:
        parser = GalleryImages()
        parser.feed(file.read_text(encoding="utf-8", errors="replace"))
        for key, item in parser.images.items():
            if key in ids:
                images.setdefault(key, item)
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    print("CSV IDs: %d | Cached pages: %d | Gallery images found: %d" %
          (len(ids), len(html_files), len(images)), flush=True)
    done = 0
    skipped = 0
    last_request = None
    for key, (card_id, ext, url) in sorted(images.items()):
        dest = output / (card_id + "." + ext)
        if dest.exists() and dest.stat().st_size > 0:
            skipped += 1
            continue
        if args.limit and done >= args.limit:
            break
        if last_request is not None:
            time.sleep(max(0, args.delay + random.uniform(0, 1.5) -
                           (time.monotonic() - last_request)))
        req = Request(url, headers={"User-Agent": "DigimonOCGResearch/1.0 (noncommercial catalog study)"})
        try:
            with urlopen(req, timeout=args.timeout) as response:
                content_type = response.headers.get("Content-Type", "").lower()
                if not content_type.startswith("image/"):
                    print("[SKIP] Not an image:", card_id, content_type, flush=True)
                    continue
                data = response.read(2 * 1024 * 1024 + 1)
                if len(data) > 2 * 1024 * 1024:
                    print("[SKIP] Over 2 MiB:", card_id, flush=True)
                    continue
            tmp = dest.with_suffix(dest.suffix + ".tmp")
            tmp.write_bytes(data)
            tmp.replace(dest)
            done += 1
            print("[%d] %s (%d bytes)" % (done, dest, len(data)), flush=True)
        except HTTPError as exc:
            print("[HTTP %s] %s" % (exc.code, card_id), flush=True)
            if exc.code in (429, 503):
                print("Rate limit/server busy: stopping. Retry later.")
                break
        except (URLError, OSError) as exc:
            print("[ERROR] %s: %s" % (card_id, exc), flush=True)
        finally:
            last_request = time.monotonic()
    print("Downloaded: %d | Already cached: %d | Missing gallery URLs: %d" %
          (done, skipped, len(ids) - len(images)))
    print("Images folder:", output)

if __name__ == "__main__":
    main()
