#!/usr/bin/env python3
# It exports all transits colors to colors.txt file.
import argparse
import json
import os.path

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    default_colors_path = os.path.dirname(os.path.abspath(__file__)) + '/../../../data/colors.txt'
    parser.add_argument('in_out_file', nargs='?', type=str, default=default_colors_path,
                        help='path to colors.txt file')
    default_transits_colors_path = os.path.dirname(os.path.abspath(__file__)) + '/../../../data/transit_colors.txt'
    parser.add_argument('-c', '--colors', nargs='?', type=str, default=default_transits_colors_path,
                        help='path to transit_colors.txt file')

    args = parser.parse_args()

    colors = set()
    with open(args.in_out_file, 'r') as in_file:
        lines = in_file.readlines()
        for l in lines:
            colors.add(int(l))
    
    fields = ['clear', 'night', 'text', 'text_night']
    with open(args.colors, 'r') as colors_file:
        tr_colors = json.load(colors_file)
        for name, color_info in tr_colors['colors'].items():
            for field in fields:
                if field in color_info:
                    colors.add(int(color_info[field], 16))
    
    with open(args.in_out_file, 'w') as out_file:
        for c in sorted(colors):
            out_file.write(str(c) + os.linesep)
