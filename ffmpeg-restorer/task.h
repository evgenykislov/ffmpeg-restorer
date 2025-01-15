#ifndef TASK_H
#define TASK_H

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>


class Task {
 public:
  Task();
  Task(const Task&);
  Task(Task&& arg);
  Task& operator=(const Task&);
  Task& operator=(Task&& arg);
  virtual ~Task();

  /*! Создать (инициализировать) задачу через аргументы ffmpeg
  \param argc, argv список аргументов командной строки, относящихся к конвертации
  \return признак, что создание прошло успешно */
  bool CreateFromArguments(int argc, char** argv);

  /*! Создать (загрузить с диска) задачу через идентификатор
  \param id идентификатор задачи, получается через другие внешние функции
  \return признак успешности создания */
  bool CreateFromID(size_t id);

  /*! Запустить задачу на выполнение. Это синхронный вызов, который делает всю
  конвертацию. Конвертацию можно прервать через Ctrl+C или другим способом
  \return признак, что вся конвертация выполнена полностью успешно */
  bool Run();

  /*! Очистить всю информацию о задаче */
  void Clear();


  /*! Получить список (текущих) задач
  \return массив со списком задач */
  static std::vector<size_t> GetTasks();

 private:

  /*! Описание одного кусочка конвертации */
  struct Chunk {
    std::filesystem::path FileName;
    size_t StartTime;
    size_t Interval;
    bool Completed;
  };


  bool is_created_; //!< Признак, что задача инициализирована/прописана
  std::filesystem::path task_cfg_path_; //!< полное имя файла с настройками по задаче

  // Сохраняемая информация по задаче
  std::filesystem::path input_file_;
  std::filesystem::path output_file_;
  bool output_file_complete_;
  std::filesystem::path list_file_;
  std::filesystem::path interim_video_file_;
  std::filesystem::path interim_data_file_;
  size_t duration_;
  std::vector<std::string> arguments_; //!< Аргументы конвертации
  std::vector<Chunk> chunks_;


  /*! Обмен данными двух экземпляров */
  void Swap(Task& arg1, Task& arg2) noexcept;

  /*! Копирование данных из одного экземпляра в другой */
  void Copy(Task& arg_to, const Task& arg_from);

  /*! Разбить конвертацию на кусочки
  TODO Описание */
  bool GenerateChunks(const std::filesystem::path& task_path);

  /*! Сгенерировать файл-список фрагментов для последующего объединения */
  bool GenerateListFile();

  /*! Выделить не-видеоданные в отдельный файл */
  // bool ExtractNonVideo();


  /*! Создать новую папку с уникальным номером в хранилище задач. Пути и размещение файлов описаны в
  notes/storage.txt. В случае ошибки содержимое возвращаемых аргументов не определено
  \param id возвращает созданный (уникальный) идентификатор задачи
  \param task_path возвращает путь для размещения всех файлов задачи
  \return признак успешного создания хранилища */
  bool CreateNewTaskStorage(size_t& id, std::filesystem::path& task_path);

  /*! Сохранить изменённое состояние задачи в конфигурационном файле
  \return признак успешной записи */
  bool Save();

};

#endif // TASK_H
