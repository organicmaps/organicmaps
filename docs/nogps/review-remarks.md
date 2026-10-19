# Замечания по кодовой базе NoGPS Maps
**Дата:** 17 октября 2026 г.  
**Ветка анализа:** `feature/nogps-rough-speed-table` / `drive/2026-10-18` / `origin/master`

---

## 1. О проекте (контекст)

Проект **NoNGPSMaps** — это специализированный форк **Organic Maps** для автономной автомобильной навигации в условиях полного отсутствия, подавления (РЭБ) или подмены (спуфинга) GPS.

- **Аппаратная часть:** внешний модуль на ESP32 (S3/C3) с IMU (гироскоп + акселерометр) и связью с CAN-шиной/OBD-II (скорость авто через ELM327). Обмен с приложением — исключительно через Bluetooth LE. Поддерживается OTA-обновление прошивки блока по BLE.
- **Ядро (`libs/map/nogps`):** счисление пути (Dead Reckoning), калибровка нуля гироскопа на остановках, компенсация задержки спидометра (`SpeedLag`), компенсация ускорений (`AccelSpeed`), привязка к дорогам (`SnapToRoad`), перенос поворотов на перекрёстки (`TurnMatcher`), сброс ошибки по изгибам дорог (`BendMatcher`), петли/развязки (`PathMatcher`), множественные метки-кандидаты (`OtherMarks`), калибровка спидометра по GPS (`SpeedTable`) и новая черновая калибровка по дорогам до появления GPS (`RoughSpeedTable`).

---

## 2. Алгоритмические замечания и ошибки в ядре C++ (`libs/map/nogps/`)

### 2.1. Отравление начальной ошибки от забракованного изгиба (`LearnRoughSpeed`)
* **Файл:** [`libs/map/nogps/inertial_navigator.cpp`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/libs/map/nogps/inertial_navigator.cpp#L293)
* **Проблема:**
  В методе `LearnRoughSpeed`:
  ```cpp
  double const errorM = calculatedM - realM;
  if (std::fabs(errorM) > calculatedM * kMaxRoughStretchError)
    return true;
  ```
  Если ошибка превышает 10% (`kMaxRoughStretchError = 0.1`), документация указывает: *«ошибка больше 10 % — это не тот изгиб или перекрёсток, отрезок не считается»*. Метод возвращает `true` (считая отрезок закрытым).
  В месте вызова (строка 1270–1271):
  ```cpp
  if (LearnRoughSpeed(*aheadM, "bend"))
    StartRoughStretch(*aheadM);
  ```
  При возврате `true` вызывается `StartRoughStretch(*aheadM)` с тем самым ошибочным `*aheadM`. Это устанавливает в качестве `m_errorM` для следующего отрезка ложное смещение не того изгиба, из-за чего последующий валидный отрезок также будет отбракован по ошибке или выучит искажённую поправку.
* **Рекомендация:**
  Если отрезок забракован (`|errorM| > calculatedM * kMaxRoughStretchError`), отрезок нужно полностью сбрасывать (`m_roughStretch.reset()`), не передавая ошибочный `*aheadM` в следующий цикл обучения.

---

### 2.2. Преждевременный сброс отрезка при сдвиге на изгибе короче 200 м
* **Файл:** [`libs/map/nogps/inertial_navigator.cpp`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/libs/map/nogps/inertial_navigator.cpp#L1293)
* **Проблема:**
  В ветке, где изгиб сдвигает позицию машины (`|aheadM| >= kMinBendShiftM`):
  ```cpp
  LearnRoughSpeed(*aheadM, "bend");
  SetRoadPosition(walked->m_position, walked->m_bearingDeg);
  ```
  Если от начала отрезка пройдено меньше 200 м (`kMinRoughStretchM`), `LearnRoughSpeed` возвращает `false` (отрезок слишком короткий и должен продолжаться до следующего изгиба). Однако следующая строка `SetRoadPosition(...)` безусловно вызывает `StartRoughStretch(0)`.
  В результате накопленный за время движения путь обнуляется, так и не завершив обучение, если изгибы со сдвигом встречаются чаще чем через 200 м.
* **Рекомендация:**
  Передавать в `SetRoadPosition` флаг или проверять, нужно ли сбрасывать черновой отрезок, если обучение отложено из-за недостаточной длины отрезка.

---

### 2.3. Повороты без переноса на перекрёсток не разрывают черновой отрезок
* **Файл:** [`libs/map/nogps/inertial_navigator.cpp`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/libs/map/nogps/inertial_navigator.cpp#L1483)
* **Проблема:**
  Согласно `navigation-rules.md`: *«Отрезок идёт от места, известного вдоль дороги (перенос поворота на перекрёсток, изгиб, GPS), до следующего изгиба»*.
  Если автомобиль совершил поворот на перекрёстке (например, 90°), но совпадение с перекрёстком не сместило метку (`places.m_chosen && places.m_chosen->m_shiftM` отсутствует), `SetRoadPosition` не вызывается. При этом `m_roughStretch` не сбрасывается.
  Черновой отрезок продолжает накапливать путь сквозь весь поворот. В повороте реальная траектория машины (дуга срезки) и геометрия графа дорог (угловатые сегменты) расходятся, из-за чего в `stretch.m_roadM` попадает чисто геометрическая погрешность поворота, искажающая вычисление ошибки скорости на следующем изгибе.
* **Рекомендация:**
  При завершении значительного поворота (`turnDeg >= TurnMatcher::kMinTurnDeg`), если он не был точно привязан к перекрёстку с известным положением, сбрасывать текущий отрезок (`m_roughStretch.reset()`), так как точное положение вдоль дороги после такого поворота не гарантировано.

---

### 2.4. Отсутствие проверки на `NaN`/`Inf` в `SpeedTable::OnDistance`
* **Файл:** [`libs/map/nogps/speed_table.cpp`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/libs/map/nogps/speed_table.cpp#L26)
* **Проблема:**
  ```cpp
  void SpeedTable::OnDistance(double carSpeedKmh, double carM, double realM)
  {
    if (carSpeedKmh < kMinSpeedKmh || carM <= 0 || realM < 0)
      return;
    ...
  ```
  По стандарту IEEE 754 любое сравнение с `NaN` возвращает `false` (`NaN <= 0` — `false`, `NaN < 0` — `false`). Если в результате крайнего случая гео-расчета или нулевого деления значение `carM` или `realM` станет `NaN`, проверка пропустит его. Значения в `m_carM` и `m_gpsM` станут `NaN`, `std::clamp` в `ScaleOf()` вернёт `NaN`, после чего `NaN` распространится на скорость и географические координаты.
* **Рекомендация:**
  Добавить явную проверку:
  ```cpp
  if (std::isnan(carM) || std::isnan(realM) || std::isinf(carM) || std::isinf(realM))
    return;
  ```

---

## 3. Замечания по состоянию Git и веткам

### 3.1. Отставание `feature/nogps-rough-speed-table` от `origin/master`
* **Проблема:**
  Текущая рабочая ветка `feature/nogps-rough-speed-table` отпочкована от коммита `41d2a08945f` (PR #4 `feature/nogps-turn-start`) **до** того, как в `origin/master` был влит PR #5 (`feature/nogps-ble-background`).
  В связи с этим в ветке отсутствуют изменения PR #5:
  - Фоновый Android-сервис [`NoGpsBoxService.java`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/android/app/src/main/java/app/organicmaps/location/NoGpsBoxService.java);
  - Обновления `AndroidManifest.xml` для работы в фоне;
  - Логика фонового удержания связи в `MwmApplication.java`.
* **Рекомендация:**
  Выполнить слияние или перебазирование ветки на актуальный `origin/master` (`git merge origin/master`), чтобы тестирование черновой таблицы скорости происходило на актуальном коде.

### 3.2. Потеря PR #1 для iOS в ветке `drive/2026-10-18`
* **Проблема:**
  Ветка подготовки к тестовой поездке `drive/2026-10-18` объединила `feature/nogps-ble-background` и `feature/nogps-rough-speed-table`, но они не содержали в своей истории PR #1 (`feature/ios-nogps-ble-background`, коммит `dd8fef1df5b`).
  В результате в `drive/2026-10-18` код для iOS (`MWMLocationManager.mm` и `Esp32BleTransport.swift`) откатился назад относительно `origin/master`.
* **Рекомендация:**
  Влить `origin/master` в ветку `drive/2026-10-18`.

### 3.3. Отставание локального `master`
* Локальный `master` находится на коммите `ddc3acfc380` (до PR #1–#5). Рекомендуется подтянуть `origin/master`: `git checkout master && git pull origin master`.

---

## 4. Замечания по Android (BLE Background Service)

### 4.1. Проверка runtime-разрешений перед запуском сервиса (Android 14 API 34)
* **Файл:** [`android/app/src/main/java/app/organicmaps/location/NoGpsBoxService.java`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/android/app/src/main/java/app/organicmaps/location/NoGpsBoxService.java#L47)
* **Проблема:**
  В методе `NoGpsBoxService.startIfCarData(context)` сервис запускается через `ContextCompat.startForegroundService(context, ...)`.
  Для сервиса с типом `connectedDevice` (`FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE`) Android 14 требует активного runtime-разрешения `BLUETOOTH_CONNECT`. Если пользователь не предоставил разрешение или отозвал его, сервис стартует, но в `onStartCommand` падает с `SecurityException` и глушится через `stopSelf()`.
* **Рекомендация:**
  Добавить проверку наличия разрешения перед вызовом `startForegroundService`:
  ```java
  if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
      ContextCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED)
  {
    return false;
  }
  ```

---

## 5. Замечания по локализации (`data/strings/strings.txt`)

### 5.1. Теги для экспорта строк в iOS
* **Файл:** [`data/strings/strings.txt`](file:///c:/Users/Roman/AndroidStudioProjects/NoNGPSMaps/data/strings/strings.txt)
* **Проблема:**
  Новые строки для отображения черновой таблицы скорости в листе датчиков:
  ```ini
  [nogps_sensors_rough_speed]
  tags = android-app
  
  [nogps_sensors_rough_speed_learning]
  tags = android-app
  ```
  помечены только тегом `tags = android-app`.
* **Рекомендация:**
  Как отмечено в `docs/nogps/ios-notes.md`, для генерации строк в iOS (`Localizable.strings`) требуется тег `tags = android-app,apple-maps`. После добавления тега необходимо перегенерировать строки.
