#!/usr/bin/env python3
import argparse
import csv
import random
import re
import time
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urljoin, urlsplit, urlunsplit
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError

class Gallery(HTMLParser):
    def __init__(self):
        super().__init__()
        self.images = {}
    def handle_starttag(self, tag, attrs):
        if tag != "img":
            return
        a = dict(attrs)
        m = re.fullmatch(r"((?:St|Bo|Bx|Ex|Sp|Pr|P|Ta|Da)-\d+[A-Za-z]*)\.(jpg|jpeg|png|webp)", a.get("alt", ""), re.I)
        src = a.get("src", "")
        if m and src.startswith("/images/thumb/"):
            self.images.setdefault(m.group(1).lower(), (m.group(1), m.group(2), urljoin("https://wikimon.net", src)))

def original_url(url):
    p = urlsplit(url)
    m = re.fullmatch(r"/images/thumb/([^/]+)/([^/]+)/([^/]+)/[^/]+", p.path)
    if not m:
        return None
    return urlunsplit((p.scheme, p.netloc, "/images/" + "/".join(m.groups()), "", ""))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", default="data/cards_crawled.csv")
    ap.add_argument("--cache", default="cache_wikimon")
    ap.add_argument("--output", default="data/card_images_original")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--delay", type=float, default=4.0)
    args = ap.parse_args()
    if args.delay < 3 or args.limit < 0:
        ap.error("delay >= 3 and limit >= 0 required")
    with open(args.csv, encoding="utf-8-sig", newline="") as f:
        ids = {row["id"].lower() for row in csv.DictReader(f)}
    images = {}
    for page in sorted(Path(args.cache).glob("*.html")):
        parser = Gallery()
        parser.feed(page.read_text(encoding="utf-8", errors="replace"))
        for key, item in parser.images.items():
            if key in ids:
                images.setdefault(key, item)
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    print("IDs:", len(ids), "Images found:", len(images), flush=True)
    count = 0
    previous = None
    for key, (card_id, ext, thumb) in sorted(images.items()):
        url = original_url(thumb)
        if not url:
            continue
        dest = output / (card_id + "." + ext)
        if dest.exists() and dest.stat().st_size:
            continue
        if args.limit and count >= args.limit:
            break
        if previous is not None:
            time.sleep(max(0, args.delay + random.uniform(0, 1.5) - (time.monotonic() - previous)))
        try:
            req = Request(url, headers={"User-Agent": "DigimonOCGResearch/1.0 (noncommercial catalog study)"})
            with urlopen(req, timeout=20) as response:
                if not response.headers.get("Content-Type", "").lower().startswith("image/"):
                    print("Not an image:", card_id, flush=True)
                    continue
                data = response.read(12 * 1024 * 1024 + 1)
            if len(data) > 12 * 1024 * 1024:
                print("Too large:", card_id, flush=True)
                continue
            tmp = dest.with_suffix(dest.suffix + ".tmp")
            tmp.write_bytes(data)
            tmp.replace(dest)
            count += 1
            print("Saved", dest, len(data), "bytes", flush=True)
        except HTTPError as exc:
            print("HTTP", exc.code, card_id, flush=True)
            if exc.code in (429, 503):
                print("Rate limited. Stopping.", flush=True)
                break
        except (URLError, OSError) as exc:
            print("Error:", card_id, exc, flush=True)
        finally:
            previous = time.monotonic()
    print("New original images:", count, "Folder:", output)

if __name__ == "__main__":
    main()
