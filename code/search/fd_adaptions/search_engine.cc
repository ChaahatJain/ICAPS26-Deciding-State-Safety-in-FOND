
#include "search_engine.h"
#include "../../exception/plaja_exception.h"
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../../utils/fd_imports/system.h"
#include "../../stats/stats_base.h"
#include "../factories/configuration.h"
#include "../fd_adaptions/timer.h"
#include "../information/property_information.h"
#include "search_statistics.h"

SearchEngine::SearchEngine(const PLAJA::Configuration& config):
    status(SearchStatus::INIT)
    , timer(nullptr)
    , timeOffset(0)
    , maxTime(config.get_int_option(PLAJA_OPTION::max_time))
    , printTimer(new Timer(config.get_option_value<unsigned int>(PLAJA_OPTION::print_time, static_cast<unsigned int>(maxTime) / 2)))
    , printStats(config.is_flag_set(PLAJA_OPTION::print_stats))
    , statsFile(config.has_value_option(PLAJA_OPTION::stats_file) ? std::make_unique<std::string>(config.get_value_option_string(PLAJA_OPTION::stats_file)) : nullptr)
    , intermediateStatsFile(statsFile and config.is_flag_set(PLAJA_OPTION::save_intermediate_stats) ? std::make_unique<std::string>(*statsFile) : nullptr)
    , engineForStats(this)
    , searchStatistics(nullptr)
    , propertyInfo(config.require_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO))
    , model(propertyInfo->get_model()) {

    if (config.has_sharable(PLAJA::SharableKey::STATS)) { searchStatistics = config.get_sharable<PLAJA::StatsBase>(PLAJA::SharableKey::STATS); }
    else {
        PLAJA_ASSERT(not config.has_sharable_ptr(PLAJA::SharableKey::STATS))
        searchStatistics = config.set_sharable<PLAJA::StatsBase>(PLAJA::SharableKey::STATS, std::make_shared<PLAJA::StatsBase>());
        config.set_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS, searchStatistics.get());
    }
    SEARCH_STATISTICS::add_stats(*searchStatistics);
    searchStatistics->set_attr_int(PLAJA::StatsInt::STATUS, SearchStatus::INIT);

}

SearchEngine::~SearchEngine() = default;

/**********************************************************************************************************************/

SearchEngine::SearchStatus SearchEngine::set_status(SearchEngine::SearchStatus status_) {
    status = status_;
    searchStatistics->set_attr_int(PLAJA::StatsInt::STATUS, status);
    return status;
}

void SearchEngine::search_internal() {
    PLAJA_ASSERT(timer) // for timeout check, but also: to prohibit non-termination, in case sub-engine does not give back control
    set_status(SearchStatus::IN_PROGRESS); // reset, necessary e.g. for sub-engines
    while (status == SearchStatus::IN_PROGRESS) { // as long as the search has not found a solution call step()

        if (timer->is_expired()) {
            PLAJA_LOG("Time limit reached. Abort search.")
            status = SearchStatus::TIMEOUT;
            break;
        }

        if (printTimer and printTimer->is_expired()) {
            update_statistics();
            set_search_time(); // intermediate updates ...

            if (printStats) {
                PLAJA_LOG("-------- Printing intermediate statistics --------")
                engineForStats->print_statistics_frame(true);
                PLAJA_LOG("--------  --------")
            }

            if (intermediateStatsFile) { engineForStats->generate_csv(); }

            printTimer->reset();
        }
        set_status(step()); // In step(), do actual search.
    }
}

void SearchEngine::search() {
    // Timer management.
    if (not timer) { timer = std::make_shared<Timer>(maxTime); }

    // Initialize data structures:
    reset_time_offset();
    set_status(initialize());
    set_initialization_time();

    // Search only if termination condition is not reached during initialization ...
    reset_time_offset();
    if (get_status() != SearchStatus::SOLVED) {
        PLAJA_LOG("Searching ...")
        search_internal();
    }
    set_search_time();

    // Finalization:
    reset_time_offset();
    if (get_status() == SearchStatus::SOLVED) { // Only finalize if actually solved.
        PLAJA_LOG("Finalizing ...")
        set_status(finalize());
    }
    set_finalization_time();

    update_statistics();
}

SearchEngine::SearchStatus SearchEngine::finalize() { return SearchStatus::FINISHED; }

/**********************************************************************************************************************/

void SearchEngine::reset_time_offset() { timeOffset = (*timer)(); }

void SearchEngine::reset_print_time() { if (printTimer) { printTimer->reset(); } }

double SearchEngine::get_engine_time() const { return (*timer)() - timeOffset; }

void SearchEngine::share_timers(SearchEngine* engine) {
    engine->timer = timer;
    engine->timeOffset = (*timer)();
}

bool SearchEngine::is_almost_expired() const { return timer->is_almost_expired(1); }

void SearchEngine::set_preprocessing_time(double time) { get_search_statistics().set_attr_double(PLAJA::StatsDouble::PREPROCESSING_TIME, time); }

void SearchEngine::set_construction_time(double time) { get_search_statistics().set_attr_double(PLAJA::StatsDouble::CONSTRUCTION_TIME, time); }

void SearchEngine::set_initialization_time() { get_search_statistics().set_attr_double(PLAJA::StatsDouble::INITIALIZATION_TIME, (*timer)() - timeOffset); }

void SearchEngine::set_search_time() { get_search_statistics().set_attr_double(PLAJA::StatsDouble::SEARCH_TIME, (*timer)() - timeOffset); }

void SearchEngine::set_finalization_time() { get_search_statistics().set_attr_double(PLAJA::StatsDouble::FINALIZATION_TIME, (*timer)() - timeOffset); }

void SearchEngine::set_time() {
    if (timer) {

        switch (get_status()) {

            case INIT: { // timeout during initialization
                static const std::string updating_string("Updating initialization time [%f]");
                PLAJA_LOG(PLAJA_UTILS::string_f(updating_string.c_str(), get_search_statistics().get_attr_double(PLAJA::StatsDouble::INITIALIZATION_TIME)));
                set_initialization_time();
                break;
            }

            case IN_PROGRESS: { // timeout during search
                static const std::string updating_string("Updating search time [%f]");
                PLAJA_LOG(PLAJA_UTILS::string_f(updating_string.c_str(), get_search_statistics().get_attr_double(PLAJA::StatsDouble::SEARCH_TIME)));
                set_search_time();
                break;
            }

            case SOLVED: { // timeout after search
                static const std::string updating_string("Updating finalization time [%f]");
                PLAJA_LOG(PLAJA_UTILS::string_f(updating_string.c_str(), get_search_statistics().get_attr_double(PLAJA::StatsDouble::FINALIZATION_TIME)));
                set_finalization_time();
                break;
            }

            default: { break; }
        }

    }

    set_status(SearchStatus::TIMEOUT); // has to happen after set_time !!!
}

/**********************************************************************************************************************/

void SearchEngine::update_statistics() const {}

void SearchEngine::print_statistics() const { searchStatistics->print_statistics(); }

void SearchEngine::print_statistics_frame(bool include_memory, const std::string& frame) const {
    if (printStats) {
        PLAJA_LOG(PLAJA_UTILS::string_f("--- STATS FRAME: %s ---", frame.c_str()));
        this->print_statistics();
        if (include_memory) { utils::print_peak_memory_in_kb(); }
        PLAJA_LOG(PLAJA_UTILS::string_f("--- STATS FRAME END: %s ---", frame.c_str()));
    }
}

void SearchEngine::stats_to_csv(std::ofstream& file) const { searchStatistics->stats_to_csv(file); }

void SearchEngine::stat_names_to_csv(std::ofstream& file) const { searchStatistics->stat_names_to_csv(file); }

//* Internal helper. */
void generate_1th_csv_row(const std::string& stats_file, const SearchEngine* search_engine) {
    std::ofstream eval_out_file(stats_file, std::ios::app);
    if (eval_out_file.fail()) { throw PlajaException(PLAJA_EXCEPTION::fileNotFoundString + stats_file); }

    eval_out_file << "property";
    search_engine->stat_names_to_csv(eval_out_file);
    eval_out_file << ",memory_peak,commandline" << std::endl;
    eval_out_file.close(); // redundant as file goes out of scope
}

void SearchEngine::generate_csv(const std::string& stats_file) const {
    PLAJA_ASSERT(propertyInfo)

    // generate 1th csv row (column names) if new:
    const bool csv_file_exist_already = PLAJA_UTILS::file_exists(stats_file);
    if (not csv_file_exist_already) { generate_1th_csv_row(stats_file, this); }

    std::ofstream eval_out_file(stats_file, std::ios::app);
    if (eval_out_file.fail()) { return; } // error, checked in generate_1th_csv_row
    eval_out_file << propertyInfo->get_property_name();
    this->stats_to_csv(eval_out_file);
    eval_out_file << PLAJA_UTILS::commaString << utils::get_peak_memory_in_kb();
    // commandline if new:
    if (not csv_file_exist_already) { eval_out_file << PLAJA_UTILS::commaString << PLAJA_GLOBAL::optionParser->get_commandline(); }
    //
    eval_out_file << std::endl;
    eval_out_file.close(); // redundant as file goes out of scope
}

void SearchEngine::generate_csv() const { if (statsFile) { generate_csv(*statsFile); } }

void SearchEngine::signal_handling() {
    PLAJA_LOG("Signal handling ...")
    set_time();
    update_statistics();
    this->print_statistics_frame(false);
    this->generate_csv();
}

void SearchEngine::trigger_intermediate_stats() {
    update_statistics();
    set_search_time();
    engineForStats->print_statistics_frame(true);
    if (intermediateStatsFile) { engineForStats->generate_csv(); }
    if (printTimer) { printTimer->reset(); }
}

