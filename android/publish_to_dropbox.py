#! /usr/bin/env python

from __future__ import print_function
from optparse import OptionParser
import os
import shutil


db_path = "/Users/jenkins/Dropbox (MapsWithMe)"
folder = "release"

def clean_dropbox():
    loc_folder = "{db_folder}/{mode_folder}".format(db_folder=db_path, mode_folder=folder)
    shutil.rmtree(loc_folder)
    os.makedirs(loc_folder)
    shutil.copy("obj.zip", loc_folder)
    shutil.copy(apk_name(), loc_folder)
    shutil.copy(ipa_name(), loc_folder)

def apk_name():
    andr_folder = "android/build/outputs/apk"
    apks = os.listdir(andr_folder)
    eligible_apks = filter(lambda x: x.endswith(".apk") and "unaligned" not in x, apks)
    return "{folder}/{apk}".format(folder=andr_folder, apk=eligible_apks[0])

def ipa_name():
    ipas = os.listdir("./")
    eligible_ipas = filter(lambda x: x.endswith(".ipa"), ipas)
    return eligible_ipas[0]

def set_globals():
    global folder
    global db_path
    usage = """The script for publishing build results to Dropbox. It takes as arguments the path to the dropbox folder (-d or --dropbox_folder_path) and the mode of the build (-m or --mode), removes all data from the dropbox_path/mode folder, recreates it anew and places the build artefacst there. The dropbox folder must exist. And it doesn't have to be a dropbox folder, it can be an ordinary folder as well.

tools/publish_to_dropbox.py -m debug -d "/Users/jenkins/Dropbox (MapsWithMe)"

"""

    parser = OptionParser(usage=usage)
    parser.add_option("-m", "--mode", dest="mode",
                      help="mode of build, i.e. debug or release", metavar="MODE")

    parser.add_option("-d", "--dropbpx_folder_path", dest="dropbox", 
                      help="path to dropbox folder", metavar="DRBOX_PATH")

    (options, args) = parser.parse_args()
    
    if options.mode is not None and options.mode in ["debug","release"]:
        folder = options.mode
    else:
        parser.print_help()
        exit(2)
    if options.dropbox is None:
        parser.print_help()
        exit(2)
    
    db_path = options.dropbox


def main():
    set_globals()
    clean_dropbox()


if __name__ == "__main__" :
    main()
