# Динамики Xiaomi Pad 5 (nabu) в Windows 11 ARM64

[![English](https://img.shields.io/badge/English-d0d7de?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHJlY3Qgd2lkdGg9IjYwIiBoZWlnaHQ9IjM2IiBmaWxsPSIjMDEyMTY5Ii8%2BPHBhdGggZD0iTTAgMCA2MCAzNk02MCAwIDAgMzYiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxMCIvPjxwYXRoIGQ9Ik0wIDAgNjAgMzZNNjAgMCAwIDM2IiBzdHJva2U9IiNDODEwMkUiIHN0cm9rZS13aWR0aD0iNCIvPjxwYXRoIGQ9Ik0zMCAwdjM2TTAgMThoNjAiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxMiIvPjxwYXRoIGQ9Ik0zMCAwdjM2TTAgMThoNjAiIHN0cm9rZT0iI0M4MTAyRSIgc3Ryb2tlLXdpZHRoPSI2Ii8%2BPC9zdmc%2B)](../README.md) [![Русский](https://img.shields.io/badge/%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-2f81f7?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHBhdGggZmlsbD0iI2ZmZiIgZD0iTTAgMGg2MHYxMkgweiIvPjxwYXRoIGZpbGw9IiMwMDM5QTYiIGQ9Ik0wIDEyaDYwdjEySDB6Ii8%2BPHBhdGggZmlsbD0iI0Q1MkIxRSIgZD0iTTAgMjRoNjB2MTJIMHoiLz48L3N2Zz4%3D)](README.ru.md)

![Статус – в разработке](https://img.shields.io/badge/status-in%20development-orange?style=for-the-badge) ![Устройство – Xiaomi Pad 5](https://img.shields.io/badge/device-Xiaomi%20Pad%205-blue?style=for-the-badge) ![Платформа – Windows 11 ARM64](https://img.shields.io/badge/platform-Windows%2011%20ARM64-0078D4?style=for-the-badge)

> [!WARNING]
> Экспериментальные аудиодрайверы предназначены только для Xiaomi Pad 5 (`nabu`) с Windows 11 ARM64.

Проект по звуку четырёх встроенных динамиков Xiaomi Pad 5 (nabu) с усилителями Cirrus Logic CS35L41.

## Требования

- **Разблокированный загрузчик** нужен для установки Windows на планшет. [Руководство по установке Windows на nabu](https://github.com/erdilS/Port-Windows-11-Xiaomi-Pad-5/blob/main/guide/English/won-deployer-install-en.md)
- **Тестовый режим Windows** (Test Mode, `TESTSIGNING`) нужен для экспериментального аудиодрайвера с тестовой подписью. После включения требуется перезапуск Windows. [Документация Microsoft о тестовом режиме](https://learn.microsoft.com/ru-ru/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)

## Скачать

| ZIP драйвера | Исходники драйвера |
| --- | --- |
| [Скачать 4.17.27.20 для ARM64](https://github.com/deskcj/xiaomi-pad5-windows11-audio/releases/download/v4.17.27.20/xiaomi-pad5-audio-4.17.27.20-arm64-driver.zip) | [Исходники разработки C/H, INF и проект EWDK](../driver/) |

В ZIP есть установка и откат через PowerShell. Подготовь четыре пары заводских файлов калибровки из HyperOS своего планшета. [Как установить](../driver/INSTALL.md)

Для скачивания: **4.17.27.20**. Исходники разработки: **4.17.27.20**. В опубликованном ZIP системных Dolby и MiSound нет.

## Статус проекта

| Функция | Статус |
| --- | --- |
| Четыре усилителя CS35L41 | ✅ .20 установлена и запущена на планшете. |
| Оригинальная обработка Dolby | ✅ Точные сравнения с оригиналом и прежние реальные SFX-тесты. |
| Переключение Dolby .21 | ✅ Реальная .21 в audiodg и ON/OFF/ON через Windows API; 47 521 кадр намеренного обхода, ноль аварийного. |
| Самостоятельный backend Dolby | ✅ LocalService, настройка SCM Automatic, установка и откат. |
| Постоянный Dolby / Atmos | ❌ Не завершены полный граф/холодный запуск, задержка/восстановление и объекты Atmos. |
| Стерео L/R | ✅ Цифровые Normal/RAW. ❌ Разделение четырёх физических динамиков не подтверждено. |
| Ресурсы/управление MiSound | ✅ Заводские данные, тесты команд/исходников и чтение ACDB. |
| MiSound в Qualcomm DSP | ❌ Не завершены реальные модули, граф, COPP и обработка. |
| Аппаратный PCM24 / качество HyperOS | ❌ Не подтверждены разрядность, акустика, сон и безопасность максимума. |

На планшете: усилители **4.17.27.20**, APO **2.0.0.4**. Экспериментальная .21 испытана и откатана; постоянная обработка ещё не завершена.

## История обновлений

- **26.11:** ✅ Оригинальный Dolby, переключение и тихий тест 3 минуты без дополнительных пропусков. ❌ После намеренного сбоя обработчика остаётся пауза около 1,8 с; постоянный запуск ещё не завершён.

[Данные проверки](results/audio-worker-scheduling-live-2026-11-26.json)

## Песочница

- Прямой Float32 .21 проверен тихим тестом; сейчас устраняются пауза восстановления и незавершённая постоянная установка.
- Дальше: убрать пропуски и завершить постоянный Dolby; MiSound DSP и физическое L/R открыты.

## ❤️ Помочь улучшить звук Windows на nabu

Поддержка помогает продолжать разработку и проверки на планшете. Можно поставить звезду репозиторию или поделиться результатами проверки звука, которые можно повторить на другом nabu.

[![Поддержать через DonationAlerts](https://img.shields.io/badge/Support-DonationAlerts-orange?style=for-the-badge)](https://www.donationalerts.com/r/deskcj)

## Связаться со мной

[![Telegram](https://img.shields.io/badge/Contact-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/deskcj) [![Написать письмо](https://img.shields.io/badge/Contact-Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:deskcj@gmail.com)

## Отказ от ответственности

Это независимый проект сообщества. Названия компаний и продуктов указывают на используемое оборудование и технологии.

Экспериментальные драйверы ядра предоставляются как есть. Перед установкой тестовой сборки сделайте резервную копию Windows и подготовьте способ восстановления.
