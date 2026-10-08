# Organic Maps — неофициальный форк/порт для ОС Аврора

Это **форк** проекта [Organic Maps](https://organicmaps.app) — неофициальный порт
на ОС Аврора. За основу взят форк для Sailfish OS
(`github.com/klahr/organicmaps-sailfish`, ветка `sailfish`); здесь он адаптирован
под Аврора ОС (`auroraapp`, Qt 5.6, RPM, валидатор Авроры).

- **Автор порта на ОС Аврора: Юрасов Леонид**
- **Форк-репозиторий: https://gitflic.ru/project/ub3gad/maps** (`git@gitflic.ru:ub3gad/maps.git`)
- **Апстрим: https://github.com/organicmaps/organicmaps**
- Это **неофициальная сборка**: она не связана с командой Organic Maps и не
  одобряется ею. Название и логотип оставлены официальными (см. лицензию).

## Лицензия и атрибуция

- Исходный код — Apache License 2.0 (`LICENSE`, `NOTICE`).
- Данные карт (`.mwm`, `.bin`) — отдельная лицензия `DATA_LICENSE.txt`. Она требует
  видимой атрибуции «Map data © OpenStreetMap and Organic Maps» со ссылками на
  <https://organicmaps.app> и <https://www.openstreetmap.org/copyright>, а также
  **запрещает white-labeling / смену брендинга** без письменного разрешения команды
  Organic Maps (`legal@organicmaps.app`). Поэтому название и иконка — официальные,
  а в «О программе» добавлена пометка о неофициальном порте и атрибуция.
- Файлы `LICENSE`, `NOTICE`, `DATA_LICENSE.txt` кладутся в пакет в
  `/usr/share/app.organicmaps.organicmaps/`.

## Отличия от порта под Sailfish

Код приложения (интерфейс, карта, логика) почти не изменён. Изменения — платформенный
слой, сборка, упаковка и совместимость с тулчейном Авроры:

- **`auroraapp` вместо `sailfishapp`**: `sailfish/app_lib.hpp` (шим `AppLib`),
  правки в `sailfish/app.cpp` и `sailfish/routing.cpp`. Главный QML задаётся явно
  (`qml/organicmaps.qml`), т.к. `Aurora::Application::pathToMainQml()` ищет
  `qml/<package_id>.qml`.
- **MLite**: `MDConfItem` → `MGConfItem` (`sailfish/helpers.cpp`).
- **Сборка/упаковка Авроры**: опция CMake `AURORA` (`cmake/OmimPlatform.cmake`,
  `sailfish/CMakeLists.txt`); ID пакета `app.organicmaps.organicmaps`; desktop-файл с
  секцией `[X-Application]` (`sailfish/organicmaps-aurora.desktop.in`); RPM spec
  `rpm/app.organicmaps.organicmaps.spec`. Sailjail-профиль для Авроры не используется.
- **Совместимость с GCC 12** (в Аврора SDK 5.2 — GCC 12.3; порт под Sailfish
  рассчитан на GCC ≥ 13):
  - `libs/routing/lanes/lane_way.{hpp,cpp}` — `std::bitset` (constexpr только с C++23)
    заменён на битовую маску;
  - `libs/opening_hours/opening_hours.hpp` — добавлен `#include <optional>`;
  - `libs/indexer/drules_format.cpp`, `libs/mwm_diff/diff.cpp` — исправлено чтение через
    `std::string::resize_and_overwrite` (libstdc++ GCC 12 передаёт ёмкость, а не
    запрошенный размер; из-за этого падал разбор `drules_*.bin`);
  - `3party/glaze` — патч от C++23 `static constexpr`-локалей и отключение `<format>`
    (см. `aurora/patches/glaze-gcc12.patch`).
- **Фикс сети**: `libs/platform/http_client_qt.{hpp,cpp}` — работа в Qt-поток теперь
  отправляется через `NetworkWorker::Post()` (кастомное `QEvent` + `QCoreApplication::postEvent`),
  а не через `QTimer::singleShot(0, worker, …)`, который из сетевого `std::thread`
  не доставлялся и HTTP-запросы не уходили. (Этот же фикс пришёл и в апстрим-обновлении
  Sailfish — совпадает по причине, отличается только механикой.)

  Root cause (проверено экспериментально): `QTimer::singleShot(0, context, functor)`
  в **Qt 5.6** строит `QSingleShotTimer` в *вызывающем* потоке и вызывает `startTimer()`
  на нём; если вызывающий поток — не `QThread`, Qt пишет
  `QObject::startTimer: Timers can only be used with threads started with QThread`,
  таймер не срабатывает и функтор не вызывается. А вызывается он из
  `Platform::Thread::Network` — это `base::DelayedThreadPool` → `std::thread`
  (`libs/base/thread_pool_delayed.hpp`, `libs/base/thread.hpp`), без Qt-диспетчера.
  Отсюда: `QNetworkReply` не создавался → соединений на :443 нет → загрузка карт «висела».

  Минимальный тест (Qt 5.6.3 vs Qt 6.10.2), вызов `QTimer::singleShot(0, receiver, fn)`:

  | Qt | из `std::thread` | из главного потока |
  |----|:---:|:---:|
  | 5.6.3 (Аврора) | **не срабатывает** (`startTimer: Timers can only be used with threads started with QThread`) | срабатывает |
  | 6.10.2 (десктоп) | срабатывает | срабатывает |

  Именно поэтому на десктопе (Qt 6) тот же код работал, а на Авроре (Qt 5.6) — нет.
  Корректные фиксы — постить событие в очередь потока-получателя:
  `QCoreApplication::postEvent` (используется в дереве, `NetworkWorker::Post`) либо
  `QMetaObject::invokeMethod(..., Qt::QueuedConnection, ...)`. Оба работают из любого потока.
- **Иконка**: заменена на официальную (`android/app/ic_launcher-playstore.png`),
  уменьшенную до 86/108/128/172.
- **Атрибуция/пометка**: добавлены в `sailfish/qml/pages/HelpPage.qml` и
  `sailfish/qml/pages/MenuPage.qml` (главное меню).

## Сборка

### Вариант 1: Аврора SDK (Docker-образ сборки)

Сборка кросс-компилятором из образа `aurora-build-tools-leo:5.2.1.110`
(или соответствующего образа AWS SDK), где есть кросс-тулчейны и sysroot с Qt 5.6.

1. Получить исходники с сабмодулями:
   ```sh
   git clone git@gitflic.ru:ub3gad/maps.git
   cd maps
   git submodule update --init --recursive 3party/boost_headers 3party/expat 3party/freetype/freetype \
       3party/harfbuzz/harfbuzz 3party/icu/icu 3party/glm 3party/fast_float 3party/utfcpp \
       3party/pugixml/pugixml 3party/zlib-ng 3party/minizip-ng 3party/BLAKE3 3party/gflags  3party/glaze 3party/fast_obj
   ```
2. Применить патч для glaze (нужен для GCC 12):
   ```sh
   git -C 3party/glaze apply ../../../aurora/patches/glaze-gcc12.patch
   ```
   (путь указывать от корня репозитория; либо `cd 3party/glaze && git apply <путь>/glaze-gcc12.patch`).
3. ARMv7 (32-битные устройства Авроры):
   ```sh
   S=/opt/cross/armv7hl-meego-linux-gnueabi/sys-root
   export PKG_CONFIG_SYSROOT_DIR=$S
   export PKG_CONFIG_LIBDIR=$S/usr/lib/pkgconfig:$S/usr/lib64/pkgconfig:$S/usr/share/pkgconfig
   cmake -S . -B build-armv7 -G Ninja \
     -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=armv7hl \
     -DCMAKE_C_COMPILER=armv7hl-meego-linux-gnueabi-gcc \
     -DCMAKE_CXX_COMPILER=armv7hl-meego-linux-gnueabi-g++ \
     -DCMAKE_FIND_ROOT_PATH=$S \
     -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
     -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
     -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
     -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
     -DCMAKE_BUILD_TYPE=Release -DAURORA=ON -DBUILD_TESTING=OFF \
     -DCMAKE_INSTALL_PREFIX=/usr
   cmake --build build-armv7 --target organicmaps_sailfish -j6
   ```
4. aarch64 (64-битные устройства):
   ```sh
   S=/opt/cross/aarch64-meego-linux-gnu/sys-root
   # то же самое с CMAKE_SYSTEM_PROCESSOR=aarch64 и компиляторами aarch64-meego-linux-gnu-gcc/g++
   ```

Примечание: в sysroot Qt 5.6 `Qt5::moc`/`rcc` указывают на ARM-бинарники, которые
не запускаются на x86_64-хосте. При кросс-сборке нужно направить их на хостовые
инструменты Qt 5.6, например:
```sh
sed -i 's#${_qt5Core_install_prefix}/lib/qt5/bin/#/usr/lib64/qt5/bin/#g' \
    $S/usr/lib/cmake/Qt5Core/Qt5CoreConfigExtras.cmake
```

### Вариант 2: RPM spec

Спецификация Авроры: `rpm/app.organicmaps.organicmaps.spec` (использует
`cmake -DAURORA=ON` и `cmake -P build/sailfish/cmake_install.cmake`). Обычно
собирается через `mb2`/`sfdk build` из Аврора SDK.

## Установка RPM

Пакет не подписан ключом Авроры, поэтому штатная установка (`rpm -i`, `pkcon`)
блокируется плагином проверки/подписи, а relocation в `/opt/app` может не сработать.
Рабочая установка в штатную раскладку `/usr`:
```sh
rpm -Uvh --replacepkgs --noplugins --nodeps app.organicmaps.organicmaps-<версия>.<арх>.rpm
```
Проверка валидатором Авроры:
```sh
rpm-validator -p regular app.organicmaps.organicmaps-<версия>.<арх>.rpm
```

## Раскладка после установки

- бинарник: `/usr/bin/app.organicmaps.organicmaps`
- ресурсы и приватная библиотека: `/usr/share/app.organicmaps.organicmaps/`
  (`data/`, `qml/`, `icons/`, `lib/liborganicmaps.so`, `sounds/`, `LICENSE`, `NOTICE`,
  `DATA_LICENSE.txt`)
- desktop: `/usr/share/applications/app.organicmaps.organicmaps.desktop`
- скачанные карты: `~/.local/share/app.organicmaps/organicmaps/<версия данных>/*.mwm`
- настройки: `~/.config/app.organicmaps/organicmaps/settings.ini`
