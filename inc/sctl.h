#pragma once

#include <tuple>
#include <variant>

namespace sctl {

template <typename S> struct State {
  using type = S;
  bool operator==(const State<S> &) const { return true; }
};

struct NoAction {};
struct KeepState : State<NoAction> {};

namespace details {

template <typename state> concept Substate = requires {
  typename state::ParentState;
};
template <typename state> concept HasStartState = requires {
  typename state::StartState;
};

} // namespace details

template <typename... States> class StateChart {
public:
  using StateActionVariant = std::variant<KeepState, State<States>...>;

  StateChart(States &...states) : _states{states...} {}

  void start(bool call_entry = false) {
    using StartingState =
        typename std::tuple_element<0, std::tuple<States...>>::type;
    using DestinationState = typename resolve_start_state<StartingState>::type;
    using StartChain = typename parent_chain<DestinationState>::type;

    if (call_entry) {
      const auto [from, to] = std::apply(
          [this](auto &&...args) {
            return call_enter_on_chain_exit_when_return(args...);
          },
          StartChain{});

      _current = from == to ? to : transition(from, to);
    } else {
      _current = State<DestinationState>{};
    }
  }

  template <typename Event> void handle(const Event &e) {
    const auto transition_to = std::visit(
        [this, &e](auto &&state) -> StateActionVariant {
          using CurrentState = std::decay<decltype(state)>::type::type;
          return call_handle_on_chain<CurrentState>(e);
        },
        _current);

    _current = transition(_current, transition_to);
  }

private:
  std::tuple<States &...> _states;
  StateActionVariant _current{KeepState{}};

  StateActionVariant transition(const StateActionVariant &from_state,
                                const StateActionVariant &to_state) {
    const auto [from, to] = std::visit(
        [this](auto &&from,
               auto &&to) -> std::pair<StateActionVariant, StateActionVariant> {
          using StateFrom = std::decay<decltype(from)>::type::type;
          using StateTo = std::decay<decltype(to)>::type::type;
          using StateToResolved = typename resolve_start_state<StateTo>::type;

          if constexpr (std::is_same<StateTo, NoAction>::value) {
            return {from, from};
          } else {
            return transition_step<StateFrom, StateToResolved>();
          }
        },
        from_state, to_state);
    if (from == to) {
      return to;
    } else {
      return transition(from, to);
    }
  }

  template <typename StateFrom, typename StateTo>
  std::pair<StateActionVariant, StateActionVariant> transition_step() {
    using FromChain = typename parent_chain<StateFrom>::type;
    using ToChain = typename parent_chain<StateTo>::type;

    using Chains = reduce_common_starting_types<FromChain, ToChain>;

    using ExitChain = tuple_or<typename Chains::type_a, State<StateFrom>>::type;
    using EnterChain = tuple_or<typename Chains::type_b, State<StateTo>>::type;

    ExitChain exit_chain{};
    EnterChain enter_chain{};

    std::apply(
        [this](auto &&...args) {
          // ugly trick to call tuple args in reverse order
          int dummy;
          ((call_exit(args), dummy) = ... = 0);
        },
        exit_chain);

    return std::apply(
        [this](auto &&...args) {
          return call_enter_on_chain_exit_when_return(args...);
        },
        enter_chain);
  }

  template <typename S, typename... Rest>
  std::pair<StateActionVariant, StateActionVariant>
  call_enter_on_chain_exit_when_return(S &&s, Rest &&...rest) {
    if constexpr (requires {
                    { call_enter(std::forward<S>(s)) }
                    ->std::convertible_to<StateActionVariant>;
                  }) {
      const auto new_state = call_enter(std::forward<S>(s));
      using NewStateType = std::decay<decltype(new_state)>::type;

      if constexpr (std::is_same<NewStateType, StateActionVariant>::value) {
        if (!std::get_if<KeepState>(&new_state)) {
          return {s, new_state};
        }
      } else if constexpr (!std::is_same<NewStateType, KeepState>::value) {
        return {s, new_state};
      }
    } else {
      call_enter(std::forward<S>(s));
    }

    if constexpr (sizeof...(Rest) > 0) {
      return call_enter_on_chain_exit_when_return<Rest...>(
          std::forward<Rest>(rest)...);
    } else {
      return {s, s};
    }
  }

  template <typename S, typename E>
  StateActionVariant call_handle_on_chain(const E &e) {
    constexpr bool can_handle = requires(S & st, const E &ev) {
      st.handle(ev);
    };

    if constexpr (can_handle) {
      return convert_to_variant(std::get<S &>(_states).handle(e));
    } else if constexpr (details::Substate<S>) {
      return call_handle_on_chain<typename S::ParentState>(e);
    } else {
      return KeepState{};
    }
  }

  template <typename StateWrapper> void call_exit(StateWrapper) {
    using S = StateWrapper::type;
    constexpr bool can_exit = requires(S & s) { s.exit(); };
    if constexpr (can_exit) {
      std::get<S &>(_states).exit();
    }
  }

  template <typename StateWrapper> auto call_enter(StateWrapper) {
    using S = StateWrapper::type;
    constexpr bool can_enter = requires(S & s) { s.enter(); };
    if constexpr (can_enter) {
      return std::get<S &>(_states).enter();
    }
  }

  template <typename... StatesSubset>
  constexpr StateActionVariant
  convert_to_variant(std::variant<StatesSubset...> v) {
    return std::visit([](auto &&arg) -> StateActionVariant { return arg; }, v);
  }

  template <typename S>
  constexpr StateActionVariant convert_to_variant(State<S> state) {
    return state;
  }

  template <typename state> struct resolve_start_state { using type = state; };
  template <details::HasStartState state>
  struct resolve_start_state<state>
      : resolve_start_state<typename state::StartState> {};

  template <typename... state> struct parent_chain;
  template <details::Substate curr, typename... tail>
  struct parent_chain<curr, tail...>
      : parent_chain<typename curr::ParentState, curr, tail...> {};
  template <typename curr, typename... tail>
  struct parent_chain<curr, tail...> {
    using type = std::tuple<State<curr>, State<tail>...>;
  };

  template <typename T1, typename T2> struct reduce_common_starting_types;
  template <typename Common, typename... A, typename... B>
  struct reduce_common_starting_types<std::tuple<Common, A...>,
                                      std::tuple<Common, B...>>
      : reduce_common_starting_types<std::tuple<A...>, std::tuple<B...>> {};
  template <typename... A, typename... B>
  struct reduce_common_starting_types<std::tuple<A...>, std::tuple<B...>> {
    using type_a = std::tuple<A...>;
    using type_b = std::tuple<B...>;
  };

  template <typename T, typename Or> struct tuple_or;
  template <typename... Ts, typename Or>
  struct tuple_or<std::tuple<Ts...>, Or> {
    using type = std::tuple<Ts...>;
  };
  template <typename Or> struct tuple_or<std::tuple<>, Or> {
    using type = std::tuple<Or>;
  };
};

} // namespace sctl
