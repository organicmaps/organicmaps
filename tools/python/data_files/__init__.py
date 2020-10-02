import os
import site
import sys


def find_data_files_in_user_installations(directory):
    possible_paths = [os.path.join(site.USER_BASE, directory),] + [
        os.path.normpath(os.path.join(p, "../../..", directory))
        for p in site.getusersitepackages()
    ]

    for p in possible_paths:
        if os.path.isdir(p):
            return p

    return None


def find_data_files_in_sys_installations(directory):
    possible_paths = [os.path.join(sys.prefix, directory),] + [
        os.path.normpath(os.path.join(p, "../../..", directory))
        for p in site.getsitepackages()
    ]
    for p in possible_paths:
        if os.path.isdir(p):
            return p

    return None


def find_data_files(directory, user_inst_first=True):
    functions = [
        (int(user_inst_first), find_data_files_in_user_installations),
        (int(not user_inst_first), find_data_files_in_sys_installations),
    ]

    functions.sort(key=lambda k: k[0])
    for prior, func in functions:
        res = func(directory)
        if res is not None:
            return res

    return None
