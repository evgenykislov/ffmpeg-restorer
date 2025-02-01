# ffmpeg-restorer
Utility runs conversion via ffmpeg utility set (installed separately) with progress tracking and crash recovery.  
  
FFmpegRR allows you to save your progress while converting and continue the process after a stop, crash, etc. from the last saved point. The utility works with conversions as “Tasks”. User adds “tasks”, the utility executes them sequentially. At any moment the work can be interrupted. A subsequent run will continue to execute the tasks from the last saved point.  

Supported OS: Linux, Windows, MacOS.

## Building:

To build the utility you will need a compiler with C++17 support and the cmake build system.  
Download the code from github or gitflic: run the command in your home directory:  
**git clone https://github.com/evgenykislov/ffmpeg-restorer.git -b main**  
Build ffmpegrr:  
**cd ffmpeg-restorer**  
**cmake -B build**  
**cmake --build build**  

## How to use:

FFmpegRR is command line utility and runs like:  
```
ffmpegrr command arguments
```  
Running the utility without arguments will continue executing tasks.

Commands:

**add** – adds a new job. The arguments are parameters for the ffmpeg conversion utility.

**flush** – clears intermediate files of completed tasks. The utility does not delete intermediate files automatically, allowing the user to check the result and rerun the conversion

**help** – displays help about the utility

**removeall** – removes all tasks (both completed and incomplete) and their intermediate files. Note: conversion results (output files) are not deleted.
  
For example:  
```  
ffmpegrr
```  
Execute added tasks  
```  
ffmpegrr add -i input.mp4 -vf “scale=iw/2:ih/2” output.mp4
```  
Add task to reduces video resolution by half on each side  

More information: https://apoheliy.com/ffmpegrr
