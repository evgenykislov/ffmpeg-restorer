#ifndef FFMPEG_H
#define FFMPEG_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class FFmpeg {
 public:
  FFmpeg();
  virtual ~FFmpeg();

  enum ProcessResult {
    kProcessError,  // Ошибка выполнения команды
    kProcessEmpty,  // Результат команды - пустой файл (нет стримов)
    kProcessSuccess  // Команда выполнилась успешно
  };

  /*! Запросить длительность медиафайла
  \param fname полный путь к файлу
  \param duration_mcs возвращаемая длительность в микросекундах
  \return признак успешности запроса длительности */
  bool RequestDuration(
      const std::filesystem::path& fname, size_t& duration_mcs);

  /*! Запросить список позиций ключевых кадров
  \param fname полный путь к файлу в нативной кодировке
  \param key_frames массив позиций (будет очищен перед заполнением)
  \return признак успешности выполнения запроса */
  bool RequestKeyFrames(
      const std::string& fname, std::vector<size_t>& key_frames);

  /*! Выполнить конвертацию фрагмента в отдельный файл. Если выходной файл
  существует, то он будет перезаписан (предполагается, что был ранее сбой в
  конвертации и получился битый файл). Функция производит детекцию пустого
  результата (выходной файл не содержит ни одного стрима)
  \param input_file имя исходного файл
  \param output_file имя выходного файла
  \param начало фрагмента для конвертации, в микросекундах
  \param длительность фрагмента для конвертации, в микросекундах
  \param input_arguments аргументы конвертации для входного файла ffmpeg
  \param arguments аргументы конвертации для выходного файла ffmpeg
  \return признак успешно сделанной конвертации */
  ProcessResult DoConvertation(std::filesystem::path input_file,
      std::filesystem::path output_file, std::optional<size_t> start_time,
      std::optional<size_t> interval,
      const std::vector<std::string>& input_arguments,
      const std::vector<std::string>& output_arguments);

  /*! Объединить видепоток с остальными потоками (звуковые, субтитры и данные)
  \param video_file файл с видеодорожкой
  \param data_file файл с остальными потоками
  \param output_file выходной файл с миксом потоков
  \return признак успешного объединения */
  bool MergeVideoAndData(std::filesystem::path video_file,
      std::filesystem::path data_file, std::filesystem::path output_file);

  /*! Объединить фрагменты видео в один файл
  \param list_file имя файл со списком фуйлов-фрагметов
  \param output_file имя суммарного файла
  \return признак успешного объединения */
  bool DoConcatenation(
      std::filesystem::path list_file, std::filesystem::path output_file);

  /*! Получить информацию о потоках в виде json-строки
  \param file файл, о котором выдаётся информация
  \return json-строка с информацией. В случае ошибки выдаётся пустая строка */
  std::string RequestStreamInfo(const std::filesystem::path& file);


 private:
  FFmpeg(const FFmpeg&) = delete;
  FFmpeg(FFmpeg&&) = delete;
  FFmpeg& operator=(const FFmpeg&) = delete;
  FFmpeg& operator=(FFmpeg&&) = delete;

  bool ParseDuration(const std::string& value, size_t& duration_mcs);

  bool ParseKeyFrames(
      const std::string& value, std::vector<size_t>& key_frames);

  /*! Попытаться по описанию ошибки детектировать случай пустого выходного файла
  \param error_description описание ошибки
  \return признак, что в описании ошибки содержится указание на пустой выходной файл */
  bool DetectEmptyOutput(const std::string& error_description);
};

#endif
