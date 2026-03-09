//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <chrono>
#include <ctime>
#include <ostream>
#include "timer.h"
#include "../../utils/floating_utils.h"

/**********************************************************************************************************************/

#define USE_STD_CHRONO // Wall clock.
// #define USE_STD_CLOCK // CPU time (https://en.cppreference.com/w/cpp/chrono/c/clock).
// #define USE_GET_TIME // CPU time (https://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html). Recommended if available.

#ifdef USE_STD_CHRONO
using TimeIntern_type = Timer::Time_type;
#else
#ifdef USE_STD_CLOCK
using TimeIntern_type = std::clock_t;
#else
using TimeIntern_type = timespec;
#endif
#endif

struct LapEntry {

    TimeIntern_type startTime;
    PLAJA::StatsBase* stats;
    PLAJA::StatsDouble attribute;

    LapEntry(TimeIntern_type start_time, PLAJA::StatsBase* stats_, PLAJA::StatsDouble attribute_):
        startTime(start_time)
        , stats(stats_)
        , attribute(attribute_) {
    }

};

namespace TIMER {

    struct TimerInternal {

        TimeIntern_type startTime;
        TimeIntern_type accumulatedTime; // NOLINT(*-use-default-member-init)
        Timer::Time_type maxTime;
        bool stopped; // NOLINT(*-use-default-member-init)

        /* To facilitate time measurement between push and pop. */
        mutable std::list<LapEntry> lapStack;

        /**************************************************************************************************************/

        inline static TimeIntern_type current_time() {
#ifdef USE_STD_CHRONO
            auto dur = std::chrono::high_resolution_clock::now().time_since_epoch();
            // auto dur_float = std::chrono::duration_cast<std::chrono::duration<TimeIntern_type>>(dur); // Does apparently not work this way.
            return static_cast<TimeIntern_type>(dur.count()) * std::chrono::high_resolution_clock::period::num / std::chrono::high_resolution_clock::period::den;
#else
#ifdef USE_STD_CLOCK
            return std::clock();
#else
            struct timespec ts; // NOLINT(*-pro-type-member-init)
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
            return ts;
#endif
#endif
        }

        inline static Timer::Time_type to_external(const TimeIntern_type& time_internal) {
#ifdef USE_STD_CHRONO
            return time_internal;
#else
#ifdef USE_STD_CLOCK
            return PLAJA_UTILS::cast_numeric<double>(time_internal) / CLOCKS_PER_SEC;
#else
            return PLAJA_UTILS::cast_numeric<Timer::Time_type>(time_internal.tv_sec) + 1e-9 * PLAJA_UTILS::cast_numeric<Timer::Time_type>(time_internal.tv_nsec); // NOLINT(*-avoid-magic-numbers)
#endif
#endif
        }

        inline static TimeIntern_type init_zero() {
#ifdef USE_GET_TIME
            timespec ts; // NOLINT(*-pro-type-member-init)
            ts.tv_sec = 0;
            ts.tv_nsec = 0;
            return ts;
#else
            return 0;
#endif
        }

        inline static TimeIntern_type add(TimeIntern_type& ts1, TimeIntern_type& ts2) {
#ifdef USE_GET_TIME
            timespec ts; // NOLINT(*-pro-type-member-init)
            ts.tv_sec = ts1.tv_sec + ts2.tv_sec;
            ts.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
            return ts;
#else
            return ts1 + ts2;
#endif
        }

        inline static TimeIntern_type diff(TimeIntern_type& ts1, TimeIntern_type& ts2) {
#ifdef USE_GET_TIME
            timespec ts; // NOLINT(*-pro-type-member-init)
            ts.tv_sec = ts1.tv_sec - ts2.tv_sec;
            ts.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
            return ts;
#else
            return ts1 - ts2;
#endif
        }

        /**************************************************************************************************************/

        explicit TimerInternal(Timer::Time_type max_time):
            startTime(current_time())
            , accumulatedTime(init_zero())
            , maxTime(max_time)
            , stopped(false) {
        }

        ~TimerInternal() = default;

        DELETE_CONSTRUCTOR(TimerInternal)

        inline TimeIntern_type get_accumulated_time() const {
            if (stopped) { return accumulatedTime; }
            else {
#ifdef USE_GET_TIME
                timespec ts = current_time();
                ts.tv_sec -= startTime.tv_sec;
                ts.tv_nsec -= startTime.tv_nsec;
                ts.tv_sec += accumulatedTime.tv_sec;
                ts.tv_nsec += accumulatedTime.tv_nsec;
                return ts;
#else
                return accumulatedTime + current_time() - startTime;
#endif
            }
        }

        inline TimeIntern_type get_lap_time(const TimeIntern_type& start_time) const {
            if (stopped) { return accumulatedTime; }
            else {
#ifdef USE_GET_TIME
                timespec ts = get_accumulated_time();
                ts.tv_sec -= start_time.tv_sec;
                ts.tv_nsec -= start_time.tv_nsec;
                return ts;
#else
                return get_accumulated_time() - start_time;
#endif
            }
        }

        inline TimeIntern_type stop() {
            accumulatedTime = get_accumulated_time();
            stopped = true;
            return accumulatedTime;
        }

        inline void resume() {
            if (stopped) {
                stopped = false;
                startTime = current_time();
            }
        }

        inline TimeIntern_type reset() {
            const auto result = get_accumulated_time();
            accumulatedTime = init_zero();
            startTime = current_time();
            return result;
        }

        inline void push_lap(PLAJA::StatsBase* stats, PLAJA::StatsDouble attribute) const {
            PLAJA_ASSERT(stats)
            lapStack.emplace_back(get_accumulated_time(), stats, attribute);
        }

        inline Timer::Time_type pop_lap() const {
            PLAJA_ASSERT(not lapStack.empty())
            auto& lap_entry = lapStack.back();
            PLAJA_ASSERT(lap_entry.stats)
            auto lap_time = to_external(get_lap_time(lap_entry.startTime));
            lap_entry.stats->inc_attr_double(lap_entry.attribute, lap_time);
            lapStack.pop_back();
            return lap_time;
        }

        inline Timer::Time_type pop_lap_intermediate() const {
            PLAJA_ASSERT(not lapStack.empty())
            auto& lap_entry = lapStack.back();
            PLAJA_ASSERT(lap_entry.stats)
            auto lap_time = to_external(get_lap_time(lap_entry.startTime));
            lap_entry.stats->set_attr_double(lap_entry.attribute, lap_time);
            lapStack.pop_back();
            return lap_time;
        }

        FCT_IF_DEBUG(void check_pop_sanity(PLAJA::StatsBase* stats, PLAJA::StatsDouble attribute) {
            PLAJA_ASSERT(not lapStack.empty())
            PLAJA_ASSERT(lapStack.back().stats == stats and lapStack.back().attribute == attribute)
        })

        inline void pop_all() const {
            for (const auto& lap_entry: lapStack) {
                PLAJA_ASSERT(lap_entry.stats)
                lap_entry.stats->inc_attr_double(lap_entry.attribute, to_external(get_lap_time(lap_entry.startTime)));
            }
            lapStack.clear();
        }

    };

}

/**********************************************************************************************************************/

Timer::Timer():
    internals(new TIMER::TimerInternal(-1)) {
}

Timer::Timer(Time_type max_time):
    internals(new TIMER::TimerInternal(max_time)) {
}

Timer::~Timer() = default;

Timer::Time_type Timer::operator()() const { return TIMER::TimerInternal::to_external(internals->get_accumulated_time()); }

Timer::Time_type Timer::stop() { return TIMER::TimerInternal::to_external(internals->stop()); }

void Timer::resume() { internals->resume(); }

Timer::Time_type Timer::reset() { return TIMER::TimerInternal::to_external(internals->reset()); }

bool Timer::is_almost_expired(Timer::Time_type tolerance) const {
    return internals->maxTime > 0 and PLAJA_FLOATS::gte(this->operator()(), internals->maxTime, tolerance);
}

bool Timer::is_expired() const { return is_almost_expired(std::numeric_limits<Time_type>::epsilon()); }

void Timer::push_lap(PLAJA::StatsBase* stats, PLAJA::StatsDouble attribute) const { internals->push_lap(stats, attribute); }

void Timer::pop_lap() const { internals->pop_lap(); }
void Timer::pop_lap_intermediate() const { internals->pop_lap_intermediate(); }

Timer::Time_type Timer::pop_lap_and_cache() const { return internals->pop_lap(); }

FCT_IF_DEBUG(void Timer::check_stack_sanity(PLAJA::StatsBase* stats, PLAJA::StatsDouble attribute) { internals->check_pop_sanity(stats, attribute); })

void Timer::pop_all() const { internals->pop_all(); }

std::ostream& operator<<(std::ostream& output, const Timer& timer) {
    output << timer() << "s";
    return output;
}
