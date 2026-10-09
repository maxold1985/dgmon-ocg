#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Synchronize and verify the offline Hyper Colosseum Starter Ver. 1+2 database.

Only merges source-reviewed local metadata; never invents individual card effects.
No network traffic. Requires Python 3 standard library only.
"""
import argparse
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATABASE = ROOT / "data" / "starter_cards.csv"
METADATA = ROOT / "data" / "starter_card_metadata.csv"
INDEX = ROOT / "data" / "starter_set_index.csv"
BASE_FIELDS = (
    "id", "set", "name_en", "name_jp", "kind", "level", "battle_type",
    "attack_a", "attack_b", "attack_c", "cancel_target", "lost_iii",
    "lost_iv", "lost_perfect", "lost_ultimate", "evolution_requirements",
    "effect_status", "source",
)
META_FIELDS = (
    "digimon_type", "attribute", "field_code", "frame", "option_type",
    "attack_a_name", "attack_b_name", "attack_c_name", "special_ability",
    "printed_bonus", "image_file", "source_set", "details_source",
    "verification_level", "notes",
)
EXTENDED_FIELDS = BASE_FIELDS + META_FIELDS

def read_rows(path):
    with path.open("r", newline="", encoding="utf-8-sig") as handle:
        reader = csv.DictReader(handle)
        headers = tuple(reader.fieldnames or ())
        rows = list(reader)
    if not rows:
        raise ValueError("Empty CSV: " + str(path))
    if any(None in row for row in rows):
        raise ValueError("Malformed CSV row in " + str(path))
    return headers, rows

def check_database():
    db_headers, base = read_rows(DATABASE)
    meta_headers, metadata = read_rows(METADATA)
    index_headers, membership = read_rows(INDEX)
    if db_headers not in (BASE_FIELDS, EXTENDED_FIELDS):
        raise ValueError("Unexpected card database schema (%d columns)" % len(db_headers))
    if meta_headers != ("id", "name_jp") + META_FIELDS:
        raise ValueError("Unexpected metadata schema")
    if index_headers != ("set", "id", "name", "kind", "level",
                         "battle_type", "attack_a", "attack_b", "attack_c", "source"):
        raise ValueError("Unexpected set index schema")
    if len(base) != 111 or len(metadata) != 111 or len(membership) != 120:
        raise ValueError("Expected 111 unique cards, 111 metadata rows and 120 memberships")
    by_id = {}
    for item in metadata:
        card_id = item["id"]
        if card_id in by_id:
            raise ValueError("Duplicate metadata ID " + card_id)
        by_id[card_id] = item
    seen = set()
    combined = []
    for row in base:
        card_id = row["id"]
        try:
            num = int(card_id[3:])
        except (ValueError, TypeError):
            raise ValueError("Invalid card ID " + str(card_id))
        if not card_id.startswith("St-") or num < 1 or num > 111:
            raise ValueError("Out-of-scope card " + card_id)
        if card_id in seen:
            raise ValueError("Duplicate card ID " + card_id)
        seen.add(card_id)
        meta = by_id.get(card_id)
        if meta is None:
            raise ValueError("Missing metadata for " + card_id)
        if meta["image_file"] != card_id + ".jpg":
            raise ValueError("Invalid image key: " + card_id)
        if not meta["name_jp"] or not row["name_en"]:
            raise ValueError("Missing card name: " + card_id)
        if row["kind"] == "Digimon":
            if not all(meta[key] for key in ("digimon_type", "attribute", "field_code")):
                raise ValueError("Incomplete Digimon classification: " + card_id)
            if meta["option_type"]:
                raise ValueError("Unexpected Option type: " + card_id)
        elif row["kind"] == "Option":
            if meta["option_type"] not in ("Item", "Program"):
                raise ValueError("Missing Option subtype: " + card_id)
        else:
            raise ValueError("Invalid card kind: " + card_id)
        normalized = {k: row[k] for k in BASE_FIELDS}
        normalized["name_jp"] = meta["name_jp"]
        normalized.update({key: meta[key] for key in META_FIELDS})
        combined.append(normalized)
    if set(by_id) != seen:
        raise ValueError("Metadata has IDs not found in database")
    for name in ("Starter Ver. 1", "Starter Ver. 2"):
        members = [r["id"] for r in membership if r["set"] == name]
        if len(members) != 60 or len(set(members)) != 60:
            raise ValueError("Wrong set membership for " + name)
        if any(id_ not in seen for id_ in members):
            raise ValueError("Membership references unknown cards in " + name)
    if set(row["id"] for row in membership) != seen:
        raise ValueError("Set membership does not cover every card")
    existing = [{k: row.get(k, "") for k in EXTENDED_FIELDS} for row in base]
    updated = existing != combined or db_headers != EXTENDED_FIELDS
    return combined, updated

def write_csv(path, records):
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=EXTENDED_FIELDS,
                                quoting=csv.QUOTE_ALL, lineterminator="\n")
        writer.writeheader()
        writer.writerows(records)
    temporary.replace(path)

def write_json(path, records):
    temporary = path.with_suffix(path.suffix + ".tmp")
    with temporary.open("w", encoding="utf-8") as handle:
        json.dump(records, handle, indent=2, ensure_ascii=False)
        handle.write("\n")
    temporary.replace(path)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply", action="store_true",
                        help="Update CSV from reviewed local metadata")
    parser.add_argument("--export-json", action="store_true",
                        help="Export enriched database as JSON")
    args = parser.parse_args()
    try:
        records, changed = check_database()
    except (OSError, ValueError, csv.Error) as exc:
        print("[ERROR]", exc, file=sys.stderr)
        return 1
    individually = sum(r["verification_level"] == "individual_page_reviewed"
                       for r in records)
    print("VALIDATED: 111 card IDs | 60 Starter 1 + 60 Starter 2 entries")
    print("Individual pages reviewed: %d | Set-list-only: %d" %
          (individually, len(records) - individually))
    print("Schema: %d fields" % len(EXTENDED_FIELDS))
    if changed:
        if not args.apply:
            print("Database needs synchronization. Run with --apply.")
            return 2
        write_csv(DATABASE, records)
        print("Updated:", DATABASE)
    else:
        print("Database already synchronized:", DATABASE)
    if args.export_json:
        dest = ROOT / "data" / "starter_cards.json"
        write_json(dest, records)
        print("Exported:", dest)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
