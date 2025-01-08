#ifndef FFMPEG_H
#define FFMPEG_H

#include <filesystem>
#include <string>
#include <vector>

class FFmpeg {
 public:
  FFmpeg();
  virtual ~FFmpeg();

  /*! Запросить длительность медиафайла
  \param fname полный путь к файлу в нативной кодировке
  \param duration_mcs возвращаемая длительность в микросекундах
  \return признак успешности запроса длительности */
  bool RequestDuration(const std::string& fname, size_t& duration_mcs);

  /*! Запросить список позиций ключевых кадров
  \param fname полный путь к файлу в нативной кодировке
  \param key_frames массив позиций (будет очищен перед заполнением)
  \return признак успешности выполнения запроса */
  bool RequestKeyFrames(const std::string& fname, std::vector<size_t>& key_frames);

  /*! Выполнить конвертацию фрагмента в отдельный файл. Если выходной файл существует,
  то он будет перезаписан (предполагается, что был ранее сбой в конвертации и получился
  битый файл).
  \param input_file исходный файл
  \param output_file выходной (результирующий) файл
  \param начало фрагмента для конвертации, в микросекундах
  \param длительность фрагмента для конвертации, в микросекундах
  \param arguments аргументы конвертации для ffmpeg
  \return признак успешно сделанной конвертации */
  bool DoConvertation(std::filesystem::path input_file,
      std::filesystem::path output_file, size_t start_time, size_t interval,
      const std::vector<std::string>& arguments);

 private:
  FFmpeg(const FFmpeg&) = delete;
  FFmpeg(FFmpeg&&) = delete;
  FFmpeg& operator=(const FFmpeg&) = delete;
  FFmpeg& operator=(FFmpeg&&) = delete;

  bool ParseDuration(const std::string& value, size_t& duration_mcs);

  bool ParseKeyFrames(const std::string& value, std::vector<size_t>& key_frames);
};

#endif
