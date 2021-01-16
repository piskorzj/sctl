#pragma once

#include <sctl.h>

#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <typeinfo>
#include <variant>
#include <vector>

// This is GCC specific!
#include <cxxabi.h>

namespace sctl::parser {

struct State {
  std::string name;
  std::optional<std::string> parent;
  std::optional<std::string> start_state;
};

struct Transition {
  std::string from;
  std::vector<std::string> to;
  std::optional<std::string> action;
};

struct ParseResult {
  std::vector<State> states;
  std::vector<Transition> transitions;
};

namespace details {

template <typename T> struct ActionReturnStates;
template <> struct ActionReturnStates<void> { using type = std::tuple<>; };
template <typename S> struct ActionReturnStates<sctl::State<S>> {
  using type = std::tuple<sctl::State<S>>;
};
template <typename... S> struct ActionReturnStates<std::variant<S...>> {
  using type = std::tuple<S...>;
};

// This is GCC specific!
template <typename T> static constexpr std::string get_typename() {
  int status;
  char *tname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
  std::string r{tname};
  free(tname);
  return r;
}

template <typename S>
static constexpr std::string get_statename(sctl::State<S>) {
  return get_typename<S>();
}

template <typename S> static constexpr std::optional<std::string> get_parent() {
  if constexpr (sctl::details::Substate<S>) {
    return get_typename<typename S::ParentState>();
  } else {
    return {};
  }
}
template <typename S>
static constexpr std::optional<std::string> get_start_state() {
  if constexpr (sctl::details::HasStartState<S>) {
    return get_typename<typename S::StartState>();
  } else {
    return {};
  }
}
template <typename S> static constexpr State get_state(sctl::State<S>) {
  return {get_typename<S>(), get_parent<S>(), get_start_state<S>()};
}

template <typename S, typename RetStates, typename A = void>
static constexpr std::vector<Transition> gate_state_transitions() {
  std::optional<std::string> action_str;
  if constexpr (!std::is_same<A, void>::value) {
    action_str = get_typename<A>();
  }

  return std::apply(
      [&action_str](auto &&...s) -> std::vector<Transition> {
        if constexpr (sizeof...(s) > 0) {
          return {{get_typename<S>(), {get_statename(s)...}, action_str}};
        } else {
          return {};
        }
      },
      typename details::ActionReturnStates<RetStates>::type{});
}

template <typename S>
static constexpr std::vector<Transition> get_state_begin_transitions() {
  if constexpr (sctl::details::HasEntryAction<S>) {
    return gate_state_transitions<S, decltype(std::declval<S>().enter())>();
  } else {
    return {};
  }
}

template <typename S, typename A>
static constexpr std::vector<Transition> get_state_action_transitions() {
  if constexpr (sctl::details::HasHandler<S, A>) {
    return gate_state_transitions<S, decltype(std::declval<S>().handle(A{})),
                                  A>();
  } else {
    return {};
  }
}

static void merge_transitions(std::vector<Transition> &dest,
                              std::vector<Transition> src) {
  dest.insert(dest.end(), std::make_move_iterator(src.begin()),
              std::make_move_iterator(src.end()));
}

template <typename A> struct Action {};
template <typename S, typename... Actions>
static constexpr std::vector<Transition> get_state_actions_transitions() {
  return std::apply(
      []<typename... A>(Action<A> && ...) {
        std::vector<Transition> r;
        (merge_transitions(r, get_state_action_transitions<S, A>()), ...);
        return r;
      },
      std::tuple<Action<Actions>...>{});
}

template <typename S, typename... Actions>
static constexpr std::vector<Transition> get_state_transitions() {
  auto r = get_state_begin_transitions<S>();
  merge_transitions(r, get_state_actions_transitions<S, Actions...>());
  return r;
}

} // namespace details

template <typename SC, typename... Actions> struct Parser;
template <typename... States, typename... Actions>
struct Parser<sctl::StateChart<States...>, Actions...> {
  static constexpr std::vector<State> get_states() {
    return std::apply(
        [](auto &&...s) {
          return std::vector<State>{details::get_state(s)...};
        },
        std::tuple<sctl::State<States>...>{});
  }

  static constexpr std::vector<Transition> get_transitions() {
    return std::apply(
        []<typename... S>(sctl::State<S> && ...) {
          std::vector<Transition> r;
          (details::merge_transitions(
               r, details::get_state_transitions<S, Actions...>()),
           ...);
          return r;
        },
        std::tuple<sctl::State<States>...>{});
  }

  ParseResult operator()() { return {get_states(), get_transitions()}; };
};

} // namespace sctl::parser
