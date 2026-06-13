#!/usr/bin/env bash

PATH=~/.local/bin:$PATH

check_ktlint() {
  return $(command -v ktlint >/dev/null 2>&1)
}

install_ktlint() {
  echo "Installing ktlint $KTLINT_VERSION"
  curl -sSLO "https://github.com/pinterest/ktlint/releases/download/${KTLINT_VERSION}/ktlint"
  chmod +x ktlint
  mkdir -p ~/.local/bin
  mv ktlint ~/.local/bin/
}

# Version is read from android/gradle/libs.versions.toml (single source of truth).
KTLINT_VERSION=$(grep -E '^ktlint\s*=' android/gradle/libs.versions.toml | grep -oE '"[^"]+"' | tr -d '"')
if [ -z "$KTLINT_VERSION" ]; then
  echo "Failed to parse ktlint version from android/gradle/libs.versions.toml"
  exit 1
fi

if ! ${check_ktlint}; then
  install_ktlint
fi

if ${check_ktlint}; then
  echo "Running ktlint version $KTLINT_VERSION"
  ktlint $@
else
  echo "ktlint not found. Please install ktlint to format Kotlin files."
fi
