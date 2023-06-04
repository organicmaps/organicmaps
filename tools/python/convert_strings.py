#!/usr/bin/env python
import argparse
import csv
import sys


def langs_order(lang):
    if lang == 'en':
        return '0'
    return lang


def read_strings(fin):
    curtitle = None
    curtrans = {}
    for line in filter(None, map(str.strip, fin)):
        if line[0].startswith('['):
            if curtrans:
                yield curtitle, curtrans
            curtitle = line.strip('[ ]')
            curtrans = {}
        elif '=' in line and curtitle:
            lang, trans = (x.strip() for x in line.split('='))
            curtrans[lang] = trans
    if curtrans:
        yield curtitle, curtrans


def from_csv(fin, fout, delim):
    r = csv.reader(fin, delimiter=delim)
    header = next(r)
    for row in r:
        fout.write('[{}]\n'.format(row[0]))
        for i, col in enumerate(map(str.strip, row)):
            if len(col) > 0 and i > 0:
                fout.write('{} = {}\n'.format(header[i], col))
        fout.write('\n')


def to_csv(fin, fout, delim, langs):
    def write_line(writer, title, translations, langs):
        row = [title]
        for lang in langs:
            row.append('' if lang not in translations else translations[lang])
        writer.writerow(row)

    w = csv.writer(fout, delimiter=delim)
    if langs is not None:
        w.writerow(['Key'] + langs)

    strings = []
    for title, trans in read_strings(fin):
        if langs is None:
            strings.append((title, trans))
        else:
            write_line(w, title, trans, langs)

    # If we don't have langs, build a list and print
    if langs is None:
        langs = set()
        for s in strings:
            langs.update(list(s[1].values()))

        langs = sorted(langs, key=langs_order)
        w.writerow(['Key'] + langs)
        for s in strings:
            write_line(w, s[0], s[1], langs)


def from_categories(fin, fout):
    raise Exception('This conversion has not been implemented yet.')


def to_categories(fin, fout):
    for title, trans in read_strings(fin):
        fout.write('{}\n'.format(title))
        for lang in sorted(trans.keys(), key=langs_order):
            fout.write('{}:^{}\n'.format(lang, trans[lang]))
        fout.write('\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Converts between strings.txt, csv files and categories.txt.')
    parser.add_argument('input', type=argparse.FileType('r'), help='Input file')
    parser.add_argument('-o', '--output', default='-', help='Output file, "-" for stdout')
    parser.add_argument('-d', '--delimiter', default=',', help='CSV delimiter')
    parser.add_argument('-l', '--langs', help='List of langs for csv: empty for default, "?" to autodetect, comma-separated for a list')
    parser.add_argument('--csv2s', action='store_true', help='CSV -> TXT')
    parser.add_argument('--s2csv', action='store_true', help='TXT -> CSV')
    parser.add_argument('--cat2s', action='store_true', help='Categories -> TXT')
    parser.add_argument('--s2cat', action='store_true', help='TXT -> Categories')
    options = parser.parse_args()

    fout = sys.stdout if options.output == '-' else open(options.output, 'w')

    if not options.langs:
        langs = 'en en-AU en-GB en-UK ar be cs da de es es-MX eu he nl fi fr hu id it ja ko nb pl pt pt-BR ro ru sk sv th tr uk vi zh-Hans zh-Hant'.split()
    elif options.langs == '?':
        langs = None
    else:
        langs = options.langs.split(',')

    if options.csv2s:
        from_csv(options.input, fout, options.delimiter)
    elif options.s2csv:
        to_csv(options.input, fout, options.delimiter, langs)
    elif options.cat2s:
        from_categories(options.input, fout)
    elif options.s2cat:
        to_categories(options.input, fout)
    else:
        raise ValueError('Please select a conversion direction.')
