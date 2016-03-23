#!/usr/bin/python
import os, sys

if len(sys.argv) < 3:
  print('This tool shows very different file sizes')
  print('Usage: {0} <newdir> <olddir> [threshold_in_%]'.format(sys.argv[0]))
  sys.exit(0)

new_path = sys.argv[1]
old_path = sys.argv[2]
threshold = (int(sys.argv[3]) if len(sys.argv) > 3 else 10) / 100.0 + 1
min_diff = 1024 * 1024

for f in sorted(os.listdir(old_path)):
  new_file = os.path.join(new_path, f)
  old_file = os.path.join(old_path, f)
  if '.mwm' not in new_file:
    continue
  if os.path.isfile(new_file) and os.path.isfile(old_file):
    new_size = os.path.getsize(new_file)
    old_size = os.path.getsize(old_file)
    if new_size + old_size > 0:
      if new_size == 0 or old_size == 0 or max(new_size, old_size) / float(min(new_size, old_size)) > threshold and abs(new_size - old_size) > min_diff:
        print('{0}: {1} {2} to {3} MB'.format(f, old_size / 1024 / 1024, 'up' if new_size > old_size else 'down', new_size / 1024 / 1024))
  else:
    print('Not found a mirror for {0}'.format(f))
