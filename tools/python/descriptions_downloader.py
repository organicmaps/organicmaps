import os
import argparse
import functools
import logging
import urllib.parse
import wikipediaapi
from multiprocessing.pool import ThreadPool
"""
This script downloads Wikipedia pages for different languages.
"""
log = logging.getLogger(__name__)

WORKERS = 16
CHUNK_SIZE = 64


def download(dir, url):
    url = urllib.parse.unquote(url)
    parsed = urllib.parse.urlparse(url)
    try:
        lang = parsed.netloc.split(".", maxsplit=1)[0]
    except (AttributeError, IndexError):
        log.exception(f"{parsed.netloc} is incorrect.")
        return None
    path = os.path.join(dir, f"{lang}.html")
    if os.path.exists(path):
        log.warning(f"{path} already exists.")
        return None
    try:
        page_name = parsed.path.rsplit("/", maxsplit=1)[-1]
    except (AttributeError, IndexError):
        log.exception(f"{parsed.path} is incorrect.")
        return None
    wiki = wikipediaapi.Wikipedia(language=lang,
                                  extract_format=wikipediaapi.ExtractFormat.HTML)
    page = wiki.page(page_name)
    text = page.text
    page_size = len(text)
    if page_size:
        references = "<h2>References</h2>"
        index = text.find(references)
        if index >= 0:
            text = text[:index] + text[index + len(references):]

        log.info(f"Save to {path} {lang} {page_name} {page_size}.")
        with open(path, "w") as file:
            file.write(text)
    else:
        log.warning(f"Page {url} is empty. It has not been saved.")
    return page


def download_all(path, url):
    page = download(path, url)
    if page is None:
        return
    try:
        lang_links = page.langlinks
    except KeyError as e:
        log.warning(f"No languages for {url} ({e}).")
        return

    for link in lang_links.values():
        download(path, link.fullurl)


def worker(output_dir):
    @functools.wraps(worker)
    def wrapped(line):
        try:
            url = line.rsplit("\t", maxsplit=1)[-1]
        except (AttributeError, IndexError):
            log.exception(f"{line} is incorrect.")
            return
        url = url.strip()
        parsed = urllib.parse.urlparse(url)
        path = os.path.join(output_dir, parsed.netloc, parsed.path[1:])
        os.makedirs(path, exist_ok=True)
        download_all(path, url)
    return wrapped


def parse_args():
    parser = argparse.ArgumentParser(description="Download wiki pages.")
    parser.add_argument("o", metavar="PATH", type=str,
                        help="Output dir for saving pages")
    parser.add_argument('--i', metavar="PATH", type=str, required=True,
                        help="Input file with wikipedia url.")
    return parser.parse_args()


def main():
    log.setLevel(logging.WARNING)
    wikipediaapi.log.setLevel(logging.WARNING)
    args = parse_args()
    input_file = args.i
    output_dir = args.o
    os.makedirs(output_dir, exist_ok=True)
    with open(input_file) as file:
        _ = file.readline()
        pool = ThreadPool(processes=WORKERS)
        pool.map(worker(output_dir), file, CHUNK_SIZE)
        pool.close()
        pool.join()


if __name__ == "__main__":
    main()
