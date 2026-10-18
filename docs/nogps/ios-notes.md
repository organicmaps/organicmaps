# Что сделать в iOS

Открытые задачи стороны iOS после изменений общего ядра `libs/map/nogps`. Сверено с кодом `master`
18 октября 2026.

Как мы работаем, кто что правит и что считается общим интерфейсом, — в
[.claude/rules/nogps-workflow.md](../../.claude/rules/nogps-workflow.md). Как ведёт себя ядро — в
[navigation-rules.md](navigation-rules.md). Что и почему менялось — в коммитах и пул-реквестах; прежний журнал
этого файла — в `git log -p -- docs/nogps/ios-notes.md`.

Когда в репозитории включат задачи (Issues), каждый пункт станет задачей с меткой `ios`, а этот файл удалится.

Строки для пунктов уже есть в `data/strings/strings.txt`. Проверка строк не пропускает метку платформы, код
которой строку не использует, поэтому с пул-реквеста №10 у строк, которых iOS ещё не показывает, нет метки
`apple-maps`. Делая пункт, добавьте её его строкам и перегенерируйте их отдельным коммитом `[strings] Regenerated`.

## Нужно сделать

1. **События ядра после `MarkNoRoad` не доходят до интерфейса.** В `MWMNoGpsEvent` (`MWMNoGps.h`) 9 значений,
   в `nogps::Event` — 14. Добавить в конец в том же порядке: `NotCalibrated`, `FirmwareUpdateAvailable`,
   `FirmwareUpdateDone`, `FirmwareUpdateFailed`, `BoxCalibrationAdvised`. Сейчас они приходят числом без имени
   и не показываются. По `NotCalibrated` — сообщение, как для `MotionSourceStopped`. Без этого пункта не
   заработают пункты 2 и 3.
2. **Просьба откалибровать блок.** Вертикаль блок сам не правит: её калибрует человек на ровной поверхности, и
   приложение просит об этом, когда блок включили заново. Кнопка `nogps_sensors_calibrate_box` уже есть. Нужно:
   - повторить `Status::m_boxCalibrationAdvised` в `MWMNoGps.h/.mm`;
   - по событию `BoxCalibrationAdvised` — вопрос: заголовок `nogps_box_calibration`, текст
     `nogps_box_calibration_offer`, кнопки `nogps_sensors_calibrate_box` (вызывает `Calibrate()` и открывает
     экран датчиков) и `later`. Ядро шлёт событие один раз на включение блока и только пока машина стоит;
   - экран датчиков, источник — блок: секция с заголовком `nogps_box_calibration` и подписью
     `nogps_box_calibration_hint`. В ней строка `nogps_sensors_box_sensors` с состоянием вместо
     `nogps_sensors_gyro`: `nogps_box_not_calibrated`, `nogps_box_calibrating` (с процентом),
     `nogps_box_calibrated`, `nogps_box_calibration_failed`, `nogps_gyro_mount_moved`; при `Done` и
     `boxCalibrationAdvised` — `nogps_box_calibration_advised`;
   - в списке «Не готово: …» для блока — `nogps_missing_box_calibration` вместо `nogps_missing_gyro`.

   Образец — Android: `SensorsBottomSheet.boxCalibrationText()`, `MwmActivity.onBoxCalibrationAdvised()`.
3. **Обновление прошивки блока из приложения.** Куски прошивки транспорт уже передаёт
   (`Esp32BleTransport.sendFirmware`), состояние `Updating` уже показывается как `nogps_box_updating`. Нужно:
   - добавить папку `data/nogps-firmware` в ресурсы приложения ссылкой на папку, как `countries-strings`. Без
     неё ядро ничего не предлагает;
   - повторить в `MWMNoGps.h/.mm` поля `Status::m_boxFirmware`, `m_bundledFirmware`, `m_firmwareUpdate`,
     `m_firmwareUpdateProgress`, `m_firmwareUpdateError` и методы `Engine::StartFirmwareUpdate()`,
     `CancelFirmwareUpdate()`;
   - экран датчиков: строка `nogps_sensors_firmware` — версия блока, а во время обновления его состояние
     (`_starting`, `_sending` с процентом, `_verifying`, `_restarting`). Кнопка `nogps_firmware_install`, когда
     `m_bundledFirmware` не пуста и не равна `m_boxFirmware`; во время обновления она же — «Отмена», кроме
     состояния `Restarting`;
   - по `FirmwareUpdateAvailable` — вопрос `nogps_firmware_title` / `nogps_firmware_offer` с кнопками
     «Установить X» и «Не сейчас»; согласие — `StartFirmwareUpdate()`. По `FirmwareUpdateDone` и
     `FirmwareUpdateFailed` — сообщения `nogps_firmware_done` и `nogps_firmware_failed` с причиной по
     `m_firmwareUpdateError`: у `MOVING`, `LOW_VOLTAGE`, `LINK`, `CANCELED`/`ABORTED`, `ROLLBACK`, `PENDING`
     свои строки, у остальных — `nogps_firmware_error_code` с кодом.

   Протокол — [esp32-firmware-spec.md](esp32-firmware-spec.md), раздел 18.
4. **Кнопка смены метки** (пул-реквест №3). Поворот, который подходит к нескольким дорогам, оставляет другие
   метки. Повторить `Status::m_otherMarks` и `Engine::SwitchMark()` в `MWMNoGps.h/.mm`. Кнопка видна, пока
   `m_otherMarks > 0`, и стоит рядом с кнопкой разворота; каждое нажатие зовёт `SwitchMark()` и показывает
   следующую метку по кругу. Подпись — `nogps_switch_mark`. На Android кнопка первая в
   ряду кнопок машины, иконка — две встречные стрелки.
5. **Черновая поправка скорости в листе датчиков** (пул-реквест №6). Повторить
   `Status::m_roughSpeedTableRanges`. Пока `m_speedTableRanges` = 0, под поправкой скорости показывать строку
   `nogps_sensors_rough_speed` с числом диапазонов черновой таблицы или, пока их 0,
   `nogps_sensors_rough_speed_learning`.

## Проверить на устройстве

- В ручном режиме двойное касание ставит метку, одиночное открывает карточку места; вне ручного режима двойное
  касание приближает карту.
- В поиске выбор результата открывает тот, на который нажали: результаты теперь идут ближними первыми.

## По желанию

- **Журнал поездки с iOS.** Включить запись журнала в файл, в листе датчиков включить «Отключить GPS», проехать
  и прислать файл. По нему `tools/python/nogps_replay.py` проверит метку; формат времени в начале строки на iOS
  может отличаться от Android, разбор под него поправим по первому файлу.
- **Акселерометр телефона.** На iOS `StartMotionSensors` пустой, в строке `TRIP` будет `phoneacc=0.00/-`. Это
  не мешает: на Android он тоже только пишется в журнал. Если понадобится: `Engine::OnAccel` ждёт оси и единицы
  Android — м/с², в покое +9,81 по оси вверх; CoreMotion отдаёт ускорение в g с обратным знаком.
