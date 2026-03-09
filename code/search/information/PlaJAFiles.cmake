set(INFORMATION_SOURCES
        # INFORMATION
        ${CMAKE_CURRENT_LIST_DIR}/model_information.cpp ${CMAKE_CURRENT_LIST_DIR}/model_information.h
        ${CMAKE_CURRENT_LIST_DIR}/property_information.cpp ${CMAKE_CURRENT_LIST_DIR}/property_information.h
        ${CMAKE_CURRENT_LIST_DIR}/synchronisation_information.cpp ${CMAKE_CURRENT_LIST_DIR}/synchronisation_information.h
        ${CMAKE_CURRENT_LIST_DIR}/racetrack/racetrack_model_info.cpp ${CMAKE_CURRENT_LIST_DIR}/racetrack/racetrack_model_info.h

        ${CMAKE_CURRENT_LIST_DIR}/jani_2_interface.h
        ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani_derived.cpp ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani_derived.h
        ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani.cpp ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani.h



        # INFORMATION NN INTERFACE
        	${CMAKE_CURRENT_LIST_DIR}/jani2nnet/using_jani2nnet.h
	        ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet.h
        	${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet_parser.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet_parser.h
        )

if (USE_VERITAS)
	list(APPEND INFORMATION_SOURCES ${CMAKE_CURRENT_LIST_DIR}/jani2ensemble/using_jani2ensemble.h
			${CMAKE_CURRENT_LIST_DIR}/jani2ensemble/jani_2_ensemble.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2ensemble/jani_2_ensemble.h
			${CMAKE_CURRENT_LIST_DIR}/jani2ensemble/jani_2_ensemble_parser.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2ensemble/jani_2_ensemble_parser.h)
endif (USE_VERITAS)
