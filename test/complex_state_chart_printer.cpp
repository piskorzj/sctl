#include "complex_state_chart.h"
#include <iostream>
#include <sctl_parser_plantuml.h>

int main() {
  sctl::parser::Parser<SC, PowerOn, PowerOff, Initialized, Failure, Action,
                       Timeout, Configure, Tick>
      parser;

  sctl::parser::PlantUML plant{parser()};

  std::cout << plant << "\n";

  return 0;
}
