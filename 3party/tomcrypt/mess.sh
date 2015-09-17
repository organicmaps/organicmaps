#!/bin/bash
if cvs log $1 >/dev/null 2>/dev/null; then exit 0; else echo "$1 shouldn't be here, removed"; rm -f $1 ; fi


