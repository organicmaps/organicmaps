#! /usr/bin/env python

from __future__ import print_function
from optparse import OptionParser
import os
from os import path
import shutil
import re


class DropBoxPublisher:
    def __init__(self):
        self.folders = list()
        self.patterns = list()
        
        self.found_files = list()
        self.loc_folder = str()


    def read_options(self):
        usage = """
        
The script for publishing build results to Dropbox. 
It takes as arguments the path to the dropbox folder (-d or --dropbox_folder_path) and the mode of the build (-m or --mode),
patterns to include (in python syntax), folders to look for files to copy, 
removes all data from the dropbox_path/mode folder, recreates it anew and places the build artefacst there. 
The dropbox folder must exist. And it doesn't have to be a dropbox folder, it can be an ordinary folder as well.
No default values are provided.

tools/publish_to_dropbox.py -m debug -d "/Users/jenkins/Dropbox (MapsWithMe)"  -p ".*\.ipa" -f "android/build/outputs/apk" -f "." -p ".*[^(unaligned)]\.apk" -p "obj.zip"
"""

        parser = OptionParser(usage=usage)
        parser.add_option("-m", "--mode", dest="mode",
                          help="mode of build, i.e. debug or release", metavar="MODE")
    
        parser.add_option("-d", "--dropbpx_folder_path", dest="dropbox", 
                          help="path to dropbox folder", metavar="DRBOX_PATH")

        parser.add_option("-p", "--pattern", dest="patterns", action="append",
                          help="regexp to filter files; files with matching names will be copied, one at a time", metavar="PATTERN")
        
        parser.add_option("-f", "--folder", dest="folders", action="append",
                          help="folders in which to look for the files to be copied, one at at time", metavar="FOLDER")

        (options, args) = parser.parse_args()
        
        if (not options.mode 
                or not options.patterns 
                or not options.folders 
                or not options.dropbox 
                or options.mode not in ["debug","release"]):
            parser.print_help()
            exit(2) 
        
        self.patterns = map(lambda x: re.compile(x), options.patterns)
        self.folders = options.folders
        self.loc_folder = path.join(options.dropbox, options.mode)

        
    def find_files(self):
        for folder in self.folders:
            apks = os.listdir(folder)
            eligible_apks = filter(lambda x: filter(lambda y: y.match(x), self.patterns), apks)
            self.found_files.extend(map(lambda x: path.join(folder, x), eligible_apks))


    def clean_dropbox(self):
        shutil.rmtree(self.loc_folder)
        os.makedirs(self.loc_folder)
        

    def copy_files_to_dropbox(self):
        for artefact in self.found_files:
            shutil.copy(artefact, self.loc_folder)
            

    def publish(self):
        self.read_options()
        self.find_files()
        self.clean_dropbox()
        self.copy_files_to_dropbox()
        

def main():
    publisher = DropBoxPublisher()
    publisher.publish()


if __name__ == "__main__" :
    main()
