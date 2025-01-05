#include <iostream>

#include "probe.h"



int main(int argc, char** argv) {

  if (argc > 1) {
    Probe p;
    size_t duration;
    if (p.RequestDuration(argv[1], duration)) {
      std::cout << "Dur-Dur: " << duration << std::endl;
    }
  }
  return 0;
}
