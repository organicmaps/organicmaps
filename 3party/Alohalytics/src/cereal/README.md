Header-only Cereal with no dependencies and no extra ```-I``` flags required.

See [examples/cereal/message.h](https://github.com/KnowSheet/3party/blob/master/examples/cereal/message.h) for example usage.

Ported by Dmitry "Dima" Korolev.

On Linux, it took the below commands. (MacOS has different ```sed -i``` setting, tweak by changing ```-i``` into ```-i ''```).

```
wget https://github.com/USCiLab/cereal/archive/v1.0.0.tar.gz
(tar xzf cereal-1.0.0.tar.gz; cp -r cereal-1.0.0/include/cereal include; rm -rf cereal-1.0.0)
find include/ -mindepth 1 -maxdepth 1 -type f | xargs sed -i "s/^#include <cereal\/\(.*\)>$/#include \".\/\1\"/"
find include/ -mindepth 2 -maxdepth 2 -type f | xargs sed -i "s/^#include <cereal\/\(.*\)>$/#include \"..\/\1\"/"
find include/ -mindepth 3 -maxdepth 3 -type f | xargs sed -i "s/^#include <cereal\/\(.*\)>$/#include \"..\/..\/\1\"/"
find include/ -mindepth 4 -maxdepth 4 -type f | xargs sed -i "s/^#include <cereal\/\(.*\)>$/#include \"..\/..\/..\/\1\"/"
```
