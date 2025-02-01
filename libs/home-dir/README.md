# home-dir
Crossplatform source code (C++11) to get path of the home directory and configuration files storage folder.
The home-dir code outputs paths as a string of char, i.e. in ASCII or UTF8 format. There are also special functions for Windows to get paths as a string of wide characters.  
Supports Windows, Linux, Mac OS.

## Usage
To get the path of the home directory, invoke the function:
```cpp
std::string HomeDirLibrary::GetHomeDir();
```

To get the path of the folder where you can store configuration files, invoke the function:
```cpp
std::string HomeDirLibrary::GetDataDir();
```

You can also use the “wide” version of the functions (Windows-only):
```cpp
std::wstring HomeDirLibrary::GetHomeDirW();
std::wstring HomeDirLibrary::GetDataDirW();
```

The functions return the requested paths without a delimiter (\ or /) at the end.  
In case of errors while getting paths, functions return empty strings.  
WARNING: functions do not check the existence or rights of returned paths.  

## Installing
You should copy the source files (src/home-dir.h, src/home-dir.cpp) to your project.

## Example of usage
There is test cmake project in 'test' folder. It shows result of usage of the functions.  

## Example of returned paths (from a test project)  
### Windows 10:
```
Home directory: 'C:\Users\username'
Local application data directory: 'C:\Users\username\AppData\Local'
```
### Linux:
```
Home directory: '/home/username'
Local application data directory: '/home/username/.config'
```
### Mac OS:
```
Home directory: '/Users/username'
Local application data directory: '/Users/username/.config'
```

## Authors

**Evgeny Kislov** - [evgenykislov.com](https://evgenykislov.com), [github/evgenykislov](https://github.com/evgenykislov)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

## Notes
The code is intentionally made as non header-only, due to including of system headers (especially Windows.h) can add some troubles to your code.  

The Windows implementation uses the version of system functions that limits the path length to MAX_PATH (about 256 characters).
