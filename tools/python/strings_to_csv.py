#!/usr/bin/env python
import argparse
import csv
import sys


class HeaderProcessor:
    def lang_from_csv(self, column):
        return column

    def lang_to_csv(self, column):
        return column

    def section_from_csv(self, title):
        return title

    def section_to_csv(self, title):
        return title


def from_csv(fin, fout, proc, delim):
    r = csv.reader(fin, delimiter=delim)
    header = next(r)
    for i in range(1, len(header)):
        header[i] = proc.lang_from_csv(header[i])
    for row in r:
        fout.write('[{}]\n'.format(proc.section_from_csv(row[0])))
        for i in range(1, len(row)):
            if len(row[i].strip()) > 0:
                fout.write('{} = {}\n'.format(header[i], row[i]))
        fout.write('\n')


def to_csv(fin, fout, proc, delim, langs):
    def write_line(writer, title, translations, langs):
        row = [title]
        for l in langs:
            row.append('' if l not in translations else translations[l])
        writer.writerow(row)

    strings = []
    w = csv.writer(fout, delimiter=delim)
    if langs is not None:
        w.writerow([''] + langs)
    curtitle = None
    curtrans = {}
    for line in fin:
        line = line.strip()
        if len(line) == 0:
            continue
        elif line[0] == '[':
            if curtrans:
                if langs is None:
                    strings.append((curtitle, curtrans))
                else:
                    write_line(w, curtitle, curtrans, langs)
            curtitle = line[1:-1]
            curtrans = {}
        elif '=' in line and curtitle:
            lang, trans = (x.strip() for x in line.split('='))
            curtrans[lang] = trans
    if curtrans:
        if langs is None:
            strings.append((curtitle, curtrans))
        else:
            write_line(w, curtitle, curtrans, langs)

    # If we don't have langs, build a list and print
    if langs is None:
        l = set()
        for s in strings:
            l.update(list(s[1].values()))

        def str_sort(k):
            if k == 'en':
                return '0'
            elif k == 'ru':
                return '1'
            return l
        l = sorted(l, key=str_sort)
        for s in strings:
            write_line(w, s[0], s[1], l)


if __name__ == '__main__':
    processors = {
        'default': HeaderProcessor()
    }

    parser = argparse.ArgumentParser(description='Coverts string.txt to csv and back')
    parser.add_argument('input', type=argparse.FileType('r'), help='Input file')
    parser.add_argument('-o', '--output', default='-', help='Output file, "-" for stdout')
    parser.add_argument('-d', '--delimiter', default=',', help='CSV delimiter')
    parser.add_argument('-l', '--langs', help='List of langs for csv: empty for default, "?" to autodetect, comma-separated for a list')
    parser.add_argument('-p', '--processor', default='default', help='Name of a header processor ({})'.format(','.join(processors.keys())))
    parser.add_argument('--csv-in', action='store_true', help='CSV -> TXT')
    parser.add_argument('--csv-out', action='store_true', help='TXT -> CSV')
    options = parser.parse_args()

    fout = sys.stdout if options.output == '-' else open(options.output, 'w')
    if options.csv_in:
        csv_in = True
    elif options.csv_out:
        csv_in = False
    else:
        raise ValueError('Autodetection is not implemented yet.')

    if csv_in:
        from_csv(options.input, fout, processors[options.processor], options.delimiter)
    else:
        if not options.langs:
            langs = 'en ru ar cs da nl fi fr de hu id it ja ko nb pl pt ro es sv th tr uk vi zh-Hans zh-Hant he sk'.split()
        elif options.langs == '?':
            langs = None
        else:
            langs = options.langs.split(',')
        to_csv(options.input, fout, processors[options.processor], options.delimiter, langs)
