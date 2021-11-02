#!/bin/bash
set -e -u -x

# Use ruby from brew on Mac OS X, because system ruby is outdated/broken/will be removed in future releases.
case $OSTYPE in
  darwin*)
    if [ -x /usr/local/opt/ruby/bin/ruby ]; then
      PATH="/usr/local/opt/ruby/bin:$PATH"
    elif [ -x /opt/homebrew/opt/ruby/bin/ruby ]; then
      PATH="/opt/homebrew/opt/ruby/bin:$PATH"
    else
      echo 'Please install Homebrew ruby by running "brew install ruby"'
      exit -1
    fi ;;
  *)
    if [ ! -x "$(which ruby)" ]; then
      echo "Please, install ruby (https://www.ruby-lang.org/en/documentation/installation/)"
      exit 1
    fi ;;
esac

OMIM_PATH="$(dirname "$0")/../.."
TWINE_SUBMODULE=tools/twine
TWINE_PATH="$OMIM_PATH/$TWINE_SUBMODULE"

if [ ! -e "$TWINE_PATH/twine" ]; then
  echo "You need to have twine submodule present to run this script"
  echo "Try 'git submodule update --init --recursive'"
  exit 1
fi

TWINE_COMMIT="$(git -C $TWINE_SUBMODULE rev-parse HEAD)"
TWINE_GEM="twine-$TWINE_COMMIT.gem"

if [ ! -f "$TWINE_PATH/$TWINE_GEM" ] || ! gem list -i twine; then
  echo "Building & installing twine gem..."
  (
    cd $TWINE_PATH \
    && rm -f *.gem \
    && gem build --output $TWINE_GEM \
    && gem install $TWINE_GEM
  )
fi

TWINE="$(gem contents twine | grep -m 1 bin/twine)"
STRINGS_FILE="$OMIM_PATH/data/strings/strings.txt"

generate(){
  # TODO: Add "--untagged --tags android" when tags are properly set.
  # TODO: Add validate-strings-file call to check for duplicates (and avoid Android build errors) when tags are properly set.
  $TWINE generate-all-localization-files --include translated --format android "$STRINGS_FILE" "$OMIM_PATH/android/res/"
  $TWINE generate-all-localization-files --format apple "$STRINGS_FILE" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
  $TWINE generate-all-localization-files --format apple-plural "$STRINGS_FILE" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
  $TWINE generate-all-localization-files --format apple --file-name InfoPlist.strings "$OMIM_PATH/iphone/plist.txt" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
  $TWINE generate-all-localization-files --format jquery "$OMIM_PATH/data/countries_names.txt" "$OMIM_PATH/data/countries-strings/"
  $TWINE generate-all-localization-files --format jquery "$OMIM_PATH/data/strings/sound.txt" "$OMIM_PATH/data/sound-strings/"
}

consume() {
  # Known problems: en-GB is re-ordered
  #$TWINE consume-all-localization-files --format android "$STRINGS_FILE" "$OMIM_PATH/android/res/"
  # Known problems: es-MX, English is added as a default for all non-translated strings
  #$TWINE consume-all-localization-files --format apple "$STRINGS_FILE" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
  # Broken: not implemented in twine
  #$TWINE consume-all-localization-files --format apple-plural "$STRINGS_FILE" "$OMIM_PATH/iphone/Maps/LocalizedStrings/"
  # Broken: --file-name is not supported
  $TWINE consume-all-localization-files --format apple --file-name InfoPlist.strings "$OMIM_PATH/iphone/plist.txt" "$OMIM_PATH/iphone/Maps/LocalizedStrings/" 
  # Broken: doesn't detect languages in JSON files properly
  #$TWINE consume-all-localization-files --format jquery "$OMIM_PATH/data/countries_names.txt" "$OMIM_PATH/data/countries-strings/"
  # Broken: doesn't detect languages in JSON files properly
  #$TWINE consume-all-localization-files --format jquery "$OMIM_PATH/data/strings/sound.txt" "$OMIM_PATH/data/sound-strings/"
}

case "${1:-generate}" in
  generate) generate;;
  consume) consume;;
  *)
    >&2 echo "Usage: $0 generate|consume"
    ;;
esac
