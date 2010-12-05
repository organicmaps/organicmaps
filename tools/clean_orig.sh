#!/bin/bash
find . -iname '*.orig' -exec rm '{}' ';'
find . -iname '*~' -exec rm '{}' ';'
find . -iname '#*#' -exec rm '{}' ';'