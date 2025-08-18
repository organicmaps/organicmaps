#!/usr/bin/env bash
set -euo pipefail

if ! command -v scour &> /dev/null; then
    echo -e "\033[1;31mScour could not be found"
    if [[ $OSTYPE == 'darwin'* ]]; then
       echo 'run command'
       echo 'brew install scour'
       echo 'to install it'
       exit
    fi
    echo 'Take a look at https://github.com/scour-project/scour'
    exit
fi

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"

echo "Started processing"
for i in style-clear/symbols style-night/symbols; do
    for f in $OMIM_PATH/data/styles/clear/$i/*.svg; do
        scour -q -i $f -o $f"-new" --enable-viewboxing --enable-id-stripping --enable-comment-stripping --strip-xml-prolog --protect-ids-noninkscape;
        mv -- "$f-new" "$f";
    done
done
echo "Done"
