#include "complex_state_chart.h"
#include <sctl_parser.h>

#include <iostream>

struct PlantUML {
  std::vector<sctl::parser::State> states;
  std::vector<sctl::parser::Transition> transitions;

  PlantUML(const sctl::parser::ParseResult &result)
      : states{result.states}, transitions{result.transitions} {}

  friend std::ostream &operator<<(std::ostream &out, const PlantUML &p) {
    out << "States:\n";
    for (const auto &s : p.states) {
      out << "\t" << s.name << " [" << s.parent.value_or("---") << "] ["
          << s.start_state.value_or("---") << "]\n";
    }
    out << "Transitions:\n";
    for (const auto &t : p.transitions) {
      out << "\t" << t.from << " --> (";
      for (const auto &d : t.to) {
        out << d << ", ";
      }
      out << ") [" << t.action.value_or("") << "]\n";
    }
    return out;
  }
};

int main() {
  sctl::parser::Parser<SC, PowerOn, PowerOff, Initialized, Failure, Action,
                       Timeout, Configure, Tick>
      parser;

  PlantUML plant{parser()};

  std::cout << plant << "\n";

  return 0;
}
