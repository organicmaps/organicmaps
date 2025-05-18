PYTHON="${PYTHON:-python3}"

function activate_venv_at_path() {
  path=$1

  if [ ! -d "$path/".venv ]; then
    "$PYTHON" -m venv "$path"/.venv
  fi

  source "$path"/.venv/bin/activate

  if [ -f "$path"/requirements.txt ]; then
    pip install --upgrade pip
    pip install -r "$path"/requirements.txt
  fi
}
