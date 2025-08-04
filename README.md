# ffmpeg-restorer
The ffmpeg-restorer utility allows you to save the progress of media file conversion (transcoding, resolution change, etc.) and continue the process from the last saved point. This eliminates the impact of possible crashes, power outages, and other factors during a long process.
The ffmpeg-restorer utility uses the popular ffmpeg pack to process media files.  
  
Supported OS: Linux, Windows, MacOS.

## Building:

To build the utility you will need a compiler with C++17 support and the cmake build system.  
Download the code from github or gitflic: run the command in your home directory:  
**git clone https://github.com/evgenykislov/ffmpeg-restorer.git -b main**  
Build ffmpegrr:  
**cd ffmpeg-restorer**  
**cmake -B build**  
**cmake --build build**  
**sudo cmake --install build**  

## Usage example:

For example, it needs to reduce the resolution of the input.mp4 video file to half and write the result to the output.mp4 file.  
Run the command:  
```
ffmpegrr add -i input.mp4 -vf “scale=iw/2:ih/2” output.mp4
```  
Wait for the message that the task has been created. It takes some time to pre-parse the file and form the task.  
After that, you can interrupt the conversion process at any time - the intermediate steps will be saved.  
To resume the conversion from the last saved point, run the utility without parameters:  
```
ffmpegrr
```  
After the conversion is complete, you can delete all intermediate files by running the command:  
```
ffmpegrr flush
```  
## Additional information:  
The following command outputs the internal help:  
```
ffmpegrr help
```  
For more information, see the website: https://apoheliy.com/ffmpegrr  
## Authors  
**Evgeny Kislov** - [evgenykislov.com](https://evgenykislov.com), [github/evgenykislov](https://github.com/evgenykislov)  
## License  
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details  
