set(FA_SEARCH_BASED_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/using_fault_analysis.h
        ${CMAKE_CURRENT_LIST_DIR}/search_statistics_fault.cpp ${CMAKE_CURRENT_LIST_DIR}/search_statistics_fault.h
        ${CMAKE_CURRENT_LIST_DIR}/policy_run_fuzzing.cpp      ${CMAKE_CURRENT_LIST_DIR}/policy_run_fuzzing.h
        ${CMAKE_CURRENT_LIST_DIR}/policy_run_sampler.cpp     ${CMAKE_CURRENT_LIST_DIR}/policy_run_sampler.h 
        
        ${CMAKE_CURRENT_LIST_DIR}/oracle.cpp      ${CMAKE_CURRENT_LIST_DIR}/oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/dsmc_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/dsmc_oracle.h

        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/tarjan_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/tarjan_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/pi_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/pi_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/pi2_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/pi2_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/pi3_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/lao_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/lao_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/lrtdp_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/infinite_oracles/lrtdp_oracle.h

        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/max_tarjan_finite_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/max_tarjan_finite_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/tarjan_scc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/vi_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/vi_oracle.h
        ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/max_lrtdp_finite_oracle.cpp ${CMAKE_CURRENT_LIST_DIR}/test_oracle/finite_oracles/max_lrtdp_finite_oracle.h

        ${CMAKE_CURRENT_LIST_DIR}/fault_engine.cpp ${CMAKE_CURRENT_LIST_DIR}/fault_engine.h
        ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/fault_finding.cpp ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/fault_finding.h
        ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/shielding.cpp ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/shielding.h
        ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/retraining_data_generation.cpp ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/retraining_data_generation.h
        ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/policy_action_fault_on_state.cpp ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/policy_action_fault_on_state.h
        ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/safe_state_manager.cpp ${CMAKE_CURRENT_LIST_DIR}/oracle_uses/safe_state_manager.h
)
