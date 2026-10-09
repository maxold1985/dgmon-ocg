#!/usr/bin/env python3
"""Build an uncompressed ZIP for the C++11 in-memory DAT reader."""
import argparse
import pathlib
import zipfile

ROOT=pathlib.Path(__file__).resolve().parent.parent
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",default="data/cards.zip")
    args=parser.parse_args()
    target=ROOT / args.output
    target.parent.mkdir(parents=True,exist_ok=True)
    sources=[(ROOT/"data"/"starter_cards.csv","starter_cards.csv")]
    for folder in ("card_images_original","card_images"):
        root=ROOT/"data"/folder
        if root.is_dir():
            for image in sorted(root.iterdir()):
                if image.is_file() and image.suffix.lower() in (".jpg",".jpeg",".png"):
                    sources.append((image,folder+"/"+image.name))
    with zipfile.ZipFile(str(target),"w",compression=zipfile.ZIP_STORED,
                         allowZip64=False) as z:
        for src,name in sources:
            z.write(str(src),name,compress_type=zipfile.ZIP_STORED)
    print("Created",target,"with",len(sources),"files (ZIP_STORED)")
if __name__=="__main__":
    main()
