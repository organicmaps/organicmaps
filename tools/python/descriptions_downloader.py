import argparse
import functools
import itertools
import logging
import os
import urllib.parse
from multiprocessing.pool import ThreadPool

import htmlmin
import wikipediaapi
from bs4 import BeautifulSoup

"""
This script downloads Wikipedia pages for different languages.
"""
log = logging.getLogger(__name__)

WORKERS = 80
CHUNK_SIZE = 128

HEADERS = {f"h{x}" for x in range(1,7)}
BAD_SECTIONS = {
    "en": ["External links", "Sources", "See also", "Bibliography", "Further reading"],
    "ru": ["Литература", "Ссылки", "См. также", "Библиография", "Примечания"],
    "es": ["Vínculos de interés", "Véase también", "Enlaces externos"]
}


def read_popularity(path):
    """
    :param path: a path of popularity file. A file contains '<id>,<rank>' rows.
    :return: a set of popularity object ids
    """
    ids = set()
    for line in open(path):
        try:
            ident = int(line.split(",", maxsplit=1)[0])
        except (AttributeError, IndexError):
            continue
        ids.add(ident)
    return ids


def should_download_wikipage(popularity_set):
    @functools.wraps(popularity_set)
    def wrapped(ident):
        return popularity_set is None or ident in popularity_set
    return wrapped


def remove_bad_sections(soup, lang):
    if lang not in BAD_SECTIONS:
        return soup

    it = iter(soup.find_all())
    current = next(it, None)
    current_header_level = None
    while current is not None:
        if current.name in HEADERS and current.text.strip() in BAD_SECTIONS[lang]:
            current_header_level = current.name
            current.extract()
            current = next(it, None)
            while current is not None:
                if current.name == current_header_level:
                    break
                current.extract()
                current = next(it, None)
        else:
            current = next(it, None)
    return soup


def beautify_page(html, lang):
    soup = BeautifulSoup(html, "html")
    for x in soup.find_all():
        if len(x.text.strip()) == 0:
            x.extract()

    soup = remove_bad_sections(soup, lang)
    html = str(soup.prettify())
    html = htmlmin.minify(html, remove_empty_space=True)
    return html


def need_lang(lang, langs):
    return lang in langs if langs else True


def download(directory, url):
    url = urllib.parse.unquote(url)
    parsed = urllib.parse.urlparse(url)
    try:
        lang = parsed.netloc.split(".", maxsplit=1)[0]
    except (AttributeError, IndexError):
        log.exception(f"{parsed.netloc} is incorrect.")
        return None
    path = os.path.join(directory, f"{lang}.html")
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
        os.makedirs(directory, exist_ok=True)
        text = beautify_page(text, lang)
        log.info(f"Save to {path} {lang} {page_name} {page_size}.")
        with open(path, "w") as file:
            file.write(text)
    else:
        log.warning(f"Page {url} is empty. It has not been saved.")
    return text


def get_wiki_langs(url):
    url = urllib.parse.unquote(url)
    parsed = urllib.parse.urlparse(url)
    try:
        lang = parsed.netloc.split(".", maxsplit=1)[0]
    except (AttributeError, IndexError):
        log.exception(f"{parsed.netloc} is incorrect.")
        return None
    wiki = wikipediaapi.Wikipedia(language=lang,
                                  extract_format=wikipediaapi.ExtractFormat.HTML)
    try:
        page_name = parsed.path.rsplit("/", maxsplit=1)[-1]
    except (AttributeError, IndexError):
        log.exception(f"{parsed.path} is incorrect.")
        return None
    page = wiki.page(page_name)
    my_lang = [(lang, url), ]
    try:
        langlinks = page.langlinks
        return list(zip(langlinks.keys(),
                        [link.fullurl for link in langlinks.values()])) + my_lang
    except KeyError as e:
        log.warning(f"No languages for {url} ({e}).")
        return my_lang


def download_all(path, url, langs):
    available_langs = get_wiki_langs(url)
    available_langs = filter(lambda x: need_lang(x[0], langs), available_langs)
    for lang in available_langs:
        download(path, lang[1])


def worker(output_dir, checker, langs):
    @functools.wraps(worker)
    def wrapped(line):
        if not line.strip():
            return

        try:
            (mwm_path, ident, url) = line.split("\t")
            ident = int(ident)
            if not checker(ident):
                return
            url = url.strip()
        except (AttributeError, IndexError):
            log.exception(f"{line} is incorrect.")
            return
        parsed = urllib.parse.urlparse(url)
        path = os.path.join(output_dir, parsed.netloc, parsed.path[1:])
        download_all(path, url, langs)
    return wrapped


def parse_args():
    parser = argparse.ArgumentParser(description="Download wiki pages.")
    parser.add_argument("--o", metavar="PATH", type=str,
                        help="Output dir for saving pages")
    parser.add_argument("--p", metavar="PATH", type=str,
                        help="File with popular object ids for which we "
                             "download wikipedia data. If not given, download "
                             "for all objects.")
    parser.add_argument('--i', metavar="PATH", type=str, required=True,
                        help="Input file with wikipedia url.")
    parser.add_argument('--langs', metavar="LANGS", type=str, nargs='+',
                        action='append',
                        help="Languages ​​for pages. If left blank, pages in all "
                             "available languages ​​will be loaded.")
    return parser.parse_args()


def main():
    log.setLevel(logging.WARNING)
    wikipediaapi.log.setLevel(logging.WARNING)
    args = parse_args()
    input_file = args.i
    output_dir = args.o
    popularity_file = args.p
    langs = list(itertools.chain.from_iterable(args.langs))
    os.makedirs(output_dir, exist_ok=True)
    popularity_set = read_popularity(popularity_file) if popularity_file else None
    if popularity_set:
        log.info(f"Popularity set size: {len(popularity_set)}.")
    checker = should_download_wikipage(popularity_set)
    with open(input_file) as file:
        _ = file.readline()
        pool = ThreadPool(processes=WORKERS)
        pool.map(worker(output_dir, checker, langs), file, CHUNK_SIZE)
        pool.close()
        pool.join()


if __name__ == "__main__":
    main()
