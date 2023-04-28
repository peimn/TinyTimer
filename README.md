# TinyTimer

# How to: program ATtiny85 via Arduino

## Add ATtiny board to Arduino IDE

- go to File > Preferences.
- in the Additional Boards Manager URLs, add:
```
http://drazzy.com/package_drazzy.com_index.json
```
- go to Tools > Board > Boards Manager.
- search and install `ATTinyCore`.

## DS3231 Library
https://github.com/NorthernWidget/DS3231/blob/master/examples/echo_time/echo_time.ino