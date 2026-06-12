#!/usr/bin/env python3

import sys
import os
import shutil

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "kothic", "src"))
import drules

DENSITIES = ['6plus', 'hdpi', 'mdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi']
VARIANTS = ['light', 'dark']


def copy_variant_resources(out_dir, suffix, target_path):
    for density in DENSITIES:
        res_path = os.path.join(out_dir, 'resources-' + density + "_design")
        if os.path.exists(res_path):
            shutil.copytree(res_path, os.path.join(target_path, 'resources-' + density + suffix))


def build_family_override(style_path, family, target_path):
    # The native reader looks an override up as a single family file (drules_<family>.bin) that packs
    # all variants, so pack the designer's per-variant single-variant builds into one. light/dark
    # share one structure and differ only in colors, which is exactly what save_binary requires.
    containers = []
    for variant in VARIANTS:
        out_dir = os.path.join(style_path, family, variant, 'out')
        drules_path = os.path.join(out_dir, 'drules_design.bin')
        if not os.path.exists(drules_path):
            print('Path {0} is not found'.format(drules_path))
            return
        containers.append(drules.load_container(drules_path))
        copy_variant_resources(out_dir, '_' + family + '_' + variant, target_path)

    try:
        drules.save_binary(os.path.join(target_path, 'drules_' + family + '.bin'), containers, VARIANTS)
    except ValueError as e:
        print('ERROR: cannot pack {0} override: {1}'.format(family, e))


if len(sys.argv) < 2:
    print('Usage: {0} <path_to_omim/data/styles> [<target_path>]'.format(sys.argv[0]))
    sys.exit()

path_to_styles = sys.argv[1]
if not os.path.isdir(path_to_styles):
    print('Invalid path to styles folder')
    sys.exit()

output_name = os.path.join('' if len(sys.argv) < 3 else sys.argv[2], 'styles')
if os.path.exists(output_name):
    shutil.rmtree(output_name)
os.makedirs(output_name)

for family in ['default', 'vehicle']:
    build_family_override(path_to_styles, family, output_name)
