#!/usr/bin/env python

import os
import sys

PREFIX_DELIMITER = '_'


def enumerate_symbols(symbols_folder_path):
    symbols = []
    for filename in os.listdir(symbols_folder_path):
        parts = os.path.splitext(filename)
        if parts[1] == ".svg":
            symbols.append(parts[0])
    return symbols


def check_symbols(symbols):
    numbers = set()
    for s in symbols:
        pos = s.find(PREFIX_DELIMITER)
        n = s[:pos]
        if pos < 0 or not n.isdigit():
            raise ValueError('Symbol ' + s + ' must have a numeric prefix')
        elif int(n) in numbers:
            raise ValueError('Symbol ' + s + ' has duplicated numeric prefix')
        else:
            numbers.add(int(n))


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: {0} <path_to_omim/data/styles> [<target_path>]'.format(sys.argv[0]))
        sys.exit(-1)

    path_to_styles = os.path.join(sys.argv[1], 'clear')
    if not os.path.isdir(path_to_styles):
        print('Invalid path to styles folder')
        sys.exit(-1)

    target_path = ''
    if len(sys.argv) >= 3:
        target_path = sys.argv[2]
    output_name = os.path.join(target_path, 'local_ads_symbols.txt');
    if os.path.exists(output_name):
        os.remove(output_name)

    paths = ['style-clear', 'style-night']
    symbols = []
    for folder_path in paths:
        s = enumerate_symbols(os.path.join(path_to_styles, folder_path, 'symbols-ad'))
        if len(symbols) != 0:
            symbols.sort()
            s.sort()
            if symbols != s:
                raise ValueError('Different symbols set in folders' + str(paths))
        else:
            symbols = s
    check_symbols(symbols)
    with open(output_name, "w") as text_file:
        for symbol in symbols:
            text_file.write(symbol + '\n')
