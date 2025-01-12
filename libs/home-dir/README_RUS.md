# home-dir
Кроссплатформенный код (C++11) для получения пути к домашней директории и папке хранения конфигурационных файлов.  
Код выдаёт пути как строку символов char, т.е. в формате ASCII или UTF8. Также для Windows реализованы отдельные функции получения путей как строки широких символов.  
Пример выдаваемых home-dir путей представлен в секции "Пример Использования".  


Код поддерживает операционные системы Windows и Linux.

## Применение
Для получения пути к домашней директории вызовите функцию:
```cpp
std::string HomeDirLibrary::GetHomeDir();
```

Для получения пути к папке, где можно хранить конфигурационные файлы, вызовите функцию:
```cpp
std::string HomeDirLibrary::GetDataDir();
```

Также в случае Windows можно использовать "широкие" версии функций:
```cpp
std::wstring HomeDirLibrary::GetHomeDirW();
std::wstring HomeDirLibrary::GetDataDirW();
```

Функции возвращают запрашиваемые пути. В случае ошибок функции возвращают пустые строки.

## Установка
Необходимо скопировать исходные файлы (src/home-dir.h, src/home-dir.cpp) в ваш проект.

## Пример использования
В папке test есть тестовый cmake-проект, показывающий применение функций.
Пример получаемого результата выполнения:

### Windows 10:
```
Home directory: 'C:\Users\username'
Local application data directory: 'C:\Users\username\AppData\Local'
Windows wide paths:
Wide Home directory: 'C:\Users\username'
Wide Local application data directory: 'C:\Users\username\AppData\Local'
```

### Linux:
```
Home directory: '/home/username'
Local application data directory: '/home/username/.config'
```
## Автор

**Evgeny Kislov** - [evgenykislov.com](https://evgenykislov.com), [github/evgenykislov](https://github.com/evgenykislov)

## Лицензия

Код распространяется под лицензией MIT - подробнее см. файл [LICENSE](LICENSE)

## Примечания
Код намеренно сделан не header-only, так как подключение системных заголовочных файлов (особенно Windows.h) может сильно осложнить использование кода.  

Реализация под Windows использует версию системных функций, ограничивающих длину пути в MAX_PATH (около 256 символов).
