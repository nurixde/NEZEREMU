# NEZER EMU

Кастомный Android-эмулятор для Windows 10/11. Внутри работает **настоящий Android** — официальный Android Emulator (AOSP) и образ Android 14 с Google Play, скачанные с серверов Google при первом запуске. Окно эмулятора встраивается в интерфейс NEZER EMU.

## Как получить готовый NezerEmuSetup.exe (без Visual Studio)

1. Зарегистрируйтесь на https://github.com (бесплатно).
2. Нажмите **+** справа сверху → **New repository** → имя `NezerEmu` → **Create repository**.
3. На странице репозитория: **uploading an existing file** (или Add file → Upload files).
4. Распакуйте архив на ПК. Откройте папку `NezerEmu` и перетащите в браузер **всё её содержимое** (папки `.github`, `src`, `assets`, `installer`, `tests`, `docs` и файлы). Папка `.github` скрытая: включите в Проводнике «Вид → Скрытые элементы».
5. Нажмите **Commit changes**.
6. Откройте вкладку **Actions**. Сборка «Build NEZER EMU (Windows)» запустится сама (если нет — выберите её и нажмите **Run workflow**). Занимает 10–20 минут.
7. Когда появится зелёная галочка: откройте сборку → внизу **Artifacts** → **NezerEmuSetup** → скачается zip с `NezerEmuSetup.exe`.
8. Если красный крестик: откройте сборку, нажмите на шаг с ошибкой и пришлите текст ошибки — её нужно исправить в коде.

Релиз с автообновлением: создайте тег `v1.0.0` (Releases → Draft a new release → Choose a tag). Сборка прикрепит EXE к релизу, а кнопка «Проверить обновления» в NEZER EMU будет его находить. Для новой версии увеличьте `VERSION` в CMakeLists.txt.

## Что происходит при установке и первом запуске

1. `NezerEmuSetup.exe`: проверка Windows 10/11 x64 и ОЗУ, установка программы и VC++ Runtime, по желанию — включение «Платформы гипервизора Windows».
2. Запуск NEZER EMU → экран блокировки → «Войти в систему» (или свайп вверх) → рабочий стол.
3. При первом входе — мастер: проверка CPU/RAM/диска/виртуализации, принятие лицензии Android SDK, загрузка около 4 ГБ (Java, SDK tools, Emulator, образ Android 14), создание устройства, `emulator -accel-check`.
4. Android запускается внутри окна. Все данные — в `%LOCALAPPDATA%\NezerEmu` (сохраняются при обновлении и удалении программы).

## Сборка на своём ПК (для разработчика)

Зависимости: Visual Studio 2022 (C++ desktop), Qt 6.7 MSVC 64-bit (модули Widgets, Network, Test), CMake 3.21+, Inno Setup 6 (для EXE).

    cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2019_64
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure
    build\Release\NezerEmu.exe

## Архитектура

| Модуль | Папка | Что делает |
|---|---|---|
| Интерфейс | `src/ui` | блокировка, рабочий стол, Dock, панель, настройки, мастер |
| Android-движок | `src/engine` | установка SDK, AVD, запуск эмулятора, ADB, консоль, диагностика APK |
| Графический вывод | `src/display` | встраивание окна Android, реальный FPS |
| Система ввода | `src/input` | Raw Input, PC Mouse Engine, профили, мультитач, задержка |
| Настройки | `src/settings` | JSON-хранилище с атомарной записью |
| Менеджер приложений | `src/apps` | реальный список приложений, ярлыки Dock |

Что работает, что прототип и что не сделано — в `docs/STATUS.md`.
