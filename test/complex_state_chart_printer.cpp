#include "complex_state_chart.h"
#include <sctl_parser.h>

#include <iostream>
#include <memory>

struct PlantUML {
  std::vector<sctl::parser::Transition> transitions;

  struct State {
    State(const sctl::parser::State &s)
        : name{s.name}, start_state{s.start_state} {}

    void add_child(std::unique_ptr<State> c) {
      children.push_back(std::move(c));
    }

    std::string name;
    std::optional<std::string> start_state;
    std::vector<std::unique_ptr<State>> children;

    void draw(std::ostream &out, size_t depth = 0) const {
      const std::string tabs(depth, '\t');
      out << tabs << "state " << name;
      if (start_state) {
        out << " {\n";
        out << tabs << "\t[*] --> " << start_state.value() << "\n";
        for (const auto &c : children) {
          c->draw(out, depth + 1);
        }
        out << tabs << "}\n";
      }
      out << tabs << "\n";
    }
  };

  std::vector<std::unique_ptr<State>> states;
  std::string start_state;

  PlantUML(const sctl::parser::ParseResult &result)
      : transitions{result.transitions} {
    std::map<std::string, sctl::parser::State> map_raw_states;
    std::map<std::string, std::unique_ptr<State>> bucket;
    start_state = result.states[0].name;
    for (const auto &s : result.states) {
      map_raw_states[s.name] = s;
      bucket.emplace(s.name, std::make_unique<State>(s));
    }

    while (!bucket.empty()) {
      for (auto it = bucket.begin(); it != bucket.end();) {
        const auto &state = map_raw_states[it->first];
        if (!state.start_state || [&map_raw_states, &bucket](auto name) {
              for (const auto &[key, value] : bucket) {
                if (map_raw_states[key].parent &&
                    name == map_raw_states[key].parent) {
                  return false;
                }
              }
              return true;
            }(state.name)) {
          if (state.parent && bucket.contains(state.parent.value())) {
            bucket[state.parent.value()]->add_child(std::move(it->second));
          } else {
            states.push_back(std::move(it->second));
          }
          bucket.erase(it++);
        } else {
          ++it;
        }
      }
    }
  }

  friend std::ostream &operator<<(std::ostream &out, const PlantUML &p) {
    out << "@startuml\n";
    for (const auto &s : p.states) {
      s->draw(out);
    }
    out << "[*] --> " << p.start_state << "\n";
    for (const auto &t : p.transitions) {
      for (const auto &d : t.to) {
        using namespace std::string_literals;
        out << t.from << " --> " << (d == "sctl::NoAction" ? t.from : d)
            << (t.action ? ": "s + t.action.value() : ""s) << "\n";
      }
    }
    out << "@enduml\n";
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
