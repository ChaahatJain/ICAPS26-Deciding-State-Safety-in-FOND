#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <fstream>
#include <memory>
#include "../../utils/default_constructors.h"
#include "../../stats/forward_stats.h"
#include "../information/forward_information.h"
#include "../factories/forward_factories.h"

class Timer;

class Model;

namespace PLAJA_UTILS { extern const std::string emptyString; }

class SearchEngine {
public:
    enum SearchStatus { INIT, IN_PROGRESS, SOLVED, FINISHED, TIMEOUT };

private:
    SearchStatus status;

    /* Time management. */
    std::shared_ptr<Timer> timer;
    double timeOffset; // Timer offset.
    double maxTime;
    std::unique_ptr<Timer> printTimer;

    /* Stats. */
    bool printStats;
    std::unique_ptr<std::string> statsFile;
    std::unique_ptr<std::string> intermediateStatsFile;
    const SearchEngine* engineForStats; // Engine for stat-outputs during search, by default this engine but possibly, e.g., a parent engine.

    SearchStatus set_status(SearchStatus status);

    /* Time management. */

    void reset_time_offset();

    void reset_print_time();

    void set_initialization_time();

    void set_search_time();

    void set_finalization_time();

    /* Stats. */

    inline PLAJA::StatsBase& get_search_statistics() { return *searchStatistics; }

protected:

    std::shared_ptr<PLAJA::StatsBase> searchStatistics;
    const PropertyInformation* propertyInfo;
    const Model* model; /* Cached from prop info. */

    /* Time management. */

    [[nodiscard]] double get_engine_time() const;

    /* Stats. */

    void trigger_intermediate_stats();

    /* Search. */

    virtual SearchStatus initialize() = 0;

    virtual SearchStatus finalize();

    virtual SearchStatus step() = 0;

public:
    explicit SearchEngine(const PLAJA::Configuration& config);
    virtual ~SearchEngine() = 0;
    DELETE_CONSTRUCTOR(SearchEngine)

    /* Init. */

    inline void set_engine_for_stats(const SearchEngine* engine_for_stats) { engineForStats = engine_for_stats ? engine_for_stats : this; }

    /* Search .*/

    [[nodiscard]] inline SearchStatus get_status() const { return status; }

    void search_internal();

    void search();

    /* Time management. */

    void share_timers(SearchEngine* engine);

    [[nodiscard]] bool is_almost_expired() const; // is engine close to timeout? (for special cases in derived engines)

    void set_preprocessing_time(double time);

    void set_construction_time(double time);

    void set_time();

    /* Stats (output). */

    /** Update statistics. May be called before printing stats or generating csv.*/
    virtual void update_statistics() const;

    virtual void print_statistics() const;

    void print_statistics_frame(bool include_memory, const std::string& frame = PLAJA_UTILS::emptyString) const;

    virtual void stats_to_csv(std::ofstream& file) const;

    virtual void stat_names_to_csv(std::ofstream& file) const;

    void generate_csv(const std::string& stats_file) const; // Note: we need modularity for sub-engine statistics.

    void generate_csv() const;

    virtual void signal_handling();

#ifndef NDEBUG

    /* For tests. */

    [[nodiscard]] inline const PLAJA::StatsBase& get_stats() const { return *searchStatistics; }

#endif

};

#endif
