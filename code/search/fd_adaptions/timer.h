//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TIMER_H
#define PLAJA_TIMER_H

#include <memory>
#include <list>
#include "../../stats/stats_base.h"
#include "../../assertions.h"

/* PlaJA: Developed out of a FastDownward class. Placed here for legacy reasons. */

namespace TIMER { struct TimerInternal; }

class Timer {
public:
    using Time_type = double;

private:
    std::unique_ptr<TIMER::TimerInternal> internals;

public:
    Timer();
    explicit Timer(Time_type max_time);
    ~Timer();
    DELETE_CONSTRUCTOR(Timer)

    Time_type operator()() const;

    Time_type stop();
    void resume();
    Time_type reset();

    [[nodiscard]] bool is_almost_expired(Time_type tolerance) const;

    [[nodiscard]] bool is_expired() const;

    /** To be called when starting a time measurement. */
    void push_lap(PLAJA::StatsBase* stats, PLAJA::StatsDouble attribute) const;

    inline void push_lap(const std::unique_ptr<PLAJA::StatsBase>& stats, PLAJA::StatsDouble attribute) const { push_lap(stats.get(), attribute); }

    inline void push_lap(const std::shared_ptr<PLAJA::StatsBase>& stats, PLAJA::StatsDouble attribute) const { push_lap(stats.get(), attribute); }

    /**  To be called when stopping a time measurement. */
    void pop_lap() const;

    void pop_lap_intermediate() const;

    [[nodiscard]] Time_type pop_lap_and_cache() const;

    FCT_IF_DEBUG(void check_stack_sanity(PLAJA::StatsBase* stats, PLAJA::StatsDouble attribute);)

    FCT_IF_DEBUG(inline void check_stack_sanity(const std::unique_ptr<PLAJA::StatsBase>& stats, PLAJA::StatsDouble attribute) { check_stack_sanity(stats.get(), attribute); })

    FCT_IF_DEBUG(inline void check_stack_sanity(const std::shared_ptr<PLAJA::StatsBase>& stats, PLAJA::StatsDouble attribute) { check_stack_sanity(stats.get(), attribute); })

    /** To be called at timeout before final stats generation. */
    void pop_all() const;

};

std::ostream& operator<<(std::ostream& output, const Timer& timer);

namespace PLAJA_GLOBAL { extern const std::unique_ptr<Timer> timer; }

#define PUSH_LAP(STATS, ATTR) { PLAJA_GLOBAL::timer->push_lap(STATS, ATTR); }
#define PUSH_LAP_IF(STATS, ATTR) if (STATS) { PUSH_LAP(STATS, ATTR) }
#ifdef NDEBUG
#define POP_LAP(STATS, ATTR) PLAJA_GLOBAL::timer->pop_lap();
#define POP_LAP_AND_CACHE(STATS, ATTR, VAR) (VAR) = PLAJA_GLOBAL::timer->pop_lap_and_cache();
#else
#define POP_LAP(STATS, ATTR) { PLAJA_GLOBAL::timer->check_stack_sanity(STATS, ATTR); PLAJA_GLOBAL::timer->pop_lap(); }
#define POP_LAP_INT(STATS, ATTR) { PLAJA_GLOBAL::timer->check_stack_sanity(STATS, ATTR); PLAJA_GLOBAL::timer->pop_lap_intermediate(); }
#define POP_LAP_AND_CACHE(STATS, ATTR, VAR) { PLAJA_GLOBAL::timer->check_stack_sanity(STATS, ATTR); (VAR) = PLAJA_GLOBAL::timer->pop_lap_and_cache(); }
#endif
#define POP_LAP_IF(STATS, ATTR) if (STATS) { POP_LAP(STATS, ATTR) }
#define POP_LAP_AND_CACHE_IF(STATS, ATTR, VAR) if (STATS) { POP_LAP_AND_CACHE(STATS, ATTR, VAR) }

#endif //PLAJA_TIMER_H