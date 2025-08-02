import os
import glob

def load_strings(file_path):
    result = {}
    if not os.path.exists(file_path):
        return result
    with open(file_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith('"') and '=' in line:
                try:
                    key, val = line.split('=', 1)
                    key = key.strip().strip('"')
                    val = val.strip().strip('";')
                    result[key] = val
                except ValueError:
                    continue
    return result

def append_missing_keys(file_path, missing_keys):
    with open(file_path, 'a', encoding='utf-8') as f:
        for key, value in missing_keys.items():
            f.write(f'"{key}" = "{value}";\n')


# Run "python3 ./tools/python/propagate_ios_l11s.py" from the root of the repository
base_dir = "./iphone/Maps/LocalizedStrings"  # Set to path where *.lproj folders are
default_lang = "en"
for filename in ["Localizable.strings", "LocalizableTypes.strings"]:
  en_file = os.path.join(base_dir, f"{default_lang}.lproj", filename)
  en_dict = load_strings(en_file)

  # Go through all localization folders
  for path in glob.glob(os.path.join(base_dir, "*.lproj")):
      lang = os.path.basename(path).replace(".lproj", "")
      if lang == default_lang:
          continue

      loc_file = os.path.join(path, filename)
      loc_dict = load_strings(loc_file)

      # Find missing keys
      missing = {k: v for k, v in en_dict.items() if k not in loc_dict}
      if missing:
          print(f"📌 Updating {lang}.lproj with {len(missing)} missing keys")
          append_missing_keys(loc_file, missing)
      else:
          print(f"✅ {lang}.lproj already has all keys")
