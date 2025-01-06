#ifndef PROBE_H
#define PROBE_H

#include <string>
#include <vector>

class Probe {
 public:
  Probe();
  virtual ~Probe();

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

 private:
  Probe(const Probe&) = delete;
  Probe(Probe&&) = delete;
  Probe& operator=(const Probe&) = delete;
  Probe& operator=(Probe&&) = delete;

  bool ParseDuration(const std::string& value, size_t& duration_mcs);

  bool ParseKeyFrames(const std::string& value, std::vector<size_t>& key_frames);
};

#endif // PROBE_H
