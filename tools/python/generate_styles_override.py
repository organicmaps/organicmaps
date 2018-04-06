#!/usr/bin/env python

import sys
import os
import shutil


def copy_style_file(style_path, drules_suffix, target_path):
    if not os.path.exists(style_path):
        print('Path {0} is not found'.format(style_path))
        return

    drules_proto_path = os.path.join(style_path, 'drules_proto_design.bin')
    if not os.path.exists(drules_proto_path):
        print('Path {0} is not found'.format(drules_proto_path))
        return
    shutil.copyfile(drules_proto_path, os.path.join(target_path, 'drules_proto' + drules_suffix + '.bin'))

    for density in ['6plus', 'hdpi', 'mdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi']:
        res_path = os.path.join(style_path, 'resources-' + density + "_design")
        if os.path.exists(res_path):
            shutil.copytree(res_path, os.path.join(target_path, 'resources-' + density + drules_suffix))


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

paths = ['clear/style-clear', 'clear/style-night', 'vehicle/style-clear', 'vehicle/style-night']
suffixes = ['_clear', '_dark', '_vehicle_clear', '_vehicle_dark']

for i in range(0, len(paths)):
    copy_style_file(os.path.join(path_to_styles, paths[i], 'out'), suffixes[i], output_name)
