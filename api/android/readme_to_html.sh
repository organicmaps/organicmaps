#!/bin/bash

set -e -u -x

## This script converts .md file to .html and opens it in browser.
## Please install next gems to use it:
## gem install redcarpet
## gem install github-markup

github-markup README.md > README.html
open README.html