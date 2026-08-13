#!/usr/bin/env bash
#
# Please run this script to configure the repository after cloning it.
#

set -euo pipefail

cd "$(dirname "$0")"

echo "Configuring the repository for development."

git submodule update --init --recursive

# Boost headers come from the small 3party/boost_headers submodule now, so an
# older clone keeps the whole boostorg/boost superproject around for nothing.
# Only report it: that checkout is a Git repository which may still hold local
# commits or patches, so deleting it automatically is not safe.
if [ -e 3party/boost ]; then
  cat <<EOF

Note: 3party/boost is not used anymore, Boost headers now come from the much
smaller 3party/boost_headers submodule. The old checkout takes about 2GB. Once
you are sure it holds nothing you need, remove it with:

  rm -rf 3party/boost $(git rev-parse --git-dir)/modules/3party/boost

EOF
fi

echo "The repository is configured for development."
