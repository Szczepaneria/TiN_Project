# System zdalnego sterowania wykorzystując urządzenie HID do gier komputerowych wzbogacone o system FPV

Remote control of RC car using gamepad or racing wheel - orchestrated by desktop app, featuring FPV stream and real time remote control.

Projekt polega na stworzeniu systemu PnP (Plug and Play) w celu sterowania autem na pilota z poziomu kierownicy gamingowej oraz dodatkowy system FPV w celu podglądu z poziomu pojazdu.

![Alt text](https://iet.agh.edu.pl/wp-content/uploads/2021/05/Logo-WIET-2021.png)

<br>
<br>
<br>


## TODO

Z racji że chcę mieć to pod ręką, tutaj będą rzeczy które trzeba rozpracować

1. **Kamera** - trzeba postawić już w pełni działający stream, czyli
    - [] zaorałem cały backend i jest wersja aplikacji z samym GUI (oraz ustawieniami) które działają, potrzebne jest *demo*
    - [] AMB-82 mini musi się łączyć, trza wykminić jak się bawimy z ustawieniami (i czy wogóle się bawimy, w ostateczności na sztywno bym założył 720p@60fps żeby uniknąć syfu jak przy 1080p)
    - [] Zmerge'ować demo z resztą alikacji

2. **Aplikacja** - co pozostało
    - [] połączyć działające demo z WinRT do szkieletu GUI, ewentualne zablokować parę ustawień korekcji skrętu i gazu
    - [] Przenieść komunikację sterowania **TYLKO** na UART - WiFi będzie mozolne, łatwiej jest wysyłać na moduł zewnętrzny
    - [] Przetestować sygnał z Kierownicy - obecna aplikacja to wykrywa, ale nie ma podglądu danych już przeprocesowanych

3. **Sterowanie** - ewentualne poprawki (już działa po WiFi)
    - [] (*SIDEQUEST*) Przenieść sterowanie jako łącze dwóch ESP32 (UART + ESP-NOW  <--> ESP-NOW + silniki)
    - [] (*SIDEQUEST*) Przelutować i przerobić istniejące trzymaki wydrukowane z PLA na PETG + konwertery napięć dolutować do płytki prototypowej

    