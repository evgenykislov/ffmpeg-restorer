#ifndef PROBE_H
#define PROBE_H

#include <string>

class Probe {
 public:
  Probe();
  virtual ~Probe();

  /*! Запросить длительность медиафайла
  \param fname полный путь к файлу в нативной кодировке
  \param duration_mcs возвращаемая длительность в микросекундах
  \return признак успешности запроса длительности */
  bool RequestDuration(const std::string& fname, size_t& duration_mcs);

 private:
  Probe(const Probe&) = delete;
  Probe(Probe&&) = delete;
  Probe& operator=(const Probe&) = delete;
  Probe& operator=(Probe&&) = delete;

  bool ParseDuration(const std::string& value, size_t& duration_mcs);
};

#endif // PROBE_H
