#!/bin/bash

# Usage: ./run_ab_experiments.sh x y num_walls skip
# Example: ./run_ab_experiments.sh 1000 10 250 950

if [ $# -ne 4 ]; then
    echo "Usage: $0 x y num_walls skip"
    exit 1
fi

x=$1
y=$2
num_walls=$3
skip=$4

# Common path base
BASE="../../dataset/archive/flappy_birds"
MODEL="${BASE}/models/flappy_birds_x${x}_y${y}_${num_walls}walls_${skip}skip.jani"
NN_DIR="${BASE}/networks/flappy_birds_x${x}_y${y}_${num_walls}walls_${skip}skip"
NN_INTERFACE="${NN_DIR}/flappy_birds_x${x}_y${y}_${num_walls}walls_${skip}skip_64_64.jani2nnet"
NN_FILE="${NN_DIR}/flappy_birds_x${x}_y${y}_${num_walls}walls_${skip}skip_64_64.nnet"
ADD_PROPS="${BASE}/additional_properties/repair/random_starts_1000/flappy_birds_x${x}_y${y}_${num_walls}walls_${skip}skip/pa_flappy_birds_x${x}_y${y}_${num_walls}walls_${skip}skip_random_starts_1000.jani"

echo "==============================================="
echo "Determining safety of states on unsafe paths (Tarjan)"
echo "==============================================="

../build/PlaJA --engine FAULT_ANALYSIS \
  --model-file "$MODEL" \
  --seed 1234 \
  --max-time 1800 \
  --nn-interface "$NN_INTERFACE" \
  --nn "$NN_FILE" \
  --initial-state-enum sample \
  --prop 1 \
  --additional-properties "$ADD_PROPS" \
  --oracle-use finding \
  --oracle-mode infinite \
  --infinite-oracle Tarjan \
  --applicability-filtering 1 \
  --fault-experiment-mode aggregate \
  --search-dir backward \
  --seconds-per-query 60 \
  --num-starts 20 \
  --save-intermediate-stats \
  --cache-fault-search \
  --distance-func avoid \
  --retraining-train-test-split 1.0 \
  --rand-prob 1 \
  --stats-file logs/oracle/tarjansafe_x${x}_y${y}_walls${num_walls}_skip${skip}.csv \
  --print-stats >> logs/oracle/tarjansafe_x${x}_y${y}_walls${num_walls}_skip${skip}.txt

echo "==============================================="
echo "Determining safety of states on unsafe paths (PI3)"
echo "==============================================="

../build/PlaJA --engine FAULT_ANALYSIS \
  --model-file "$MODEL" \
  --seed 1234 \
  --max-time 1800 \
  --nn-interface "$NN_INTERFACE" \
  --nn "$NN_FILE" \
  --initial-state-enum sample \
  --prop 1 \
  --additional-properties "$ADD_PROPS" \
  --oracle-use finding \
  --oracle-mode infinite \
  --infinite-oracle PI3 \
  --applicability-filtering 1 \
  --fault-experiment-mode aggregate \
  --search-dir backward \
  --seconds-per-query 60 \
  --num-starts 100 \
  --save-intermediate-stats \
  --distance-func avoid \
  --cache-fault-search \
  --retraining-train-test-split 1.0 \
  --rand-prob 1 \
  --stats-file logs/oracle/pi3_x${x}_y${y}_walls${num_walls}_skip${skip}.csv \
  --print-stats >> logs/oracle/pi3_x${x}_y${y}_walls${num_walls}_skip${skip}.txt


# echo "==============================================="
# echo "Running FAULT_ANALYSIS (LRTDP)"
# echo "==============================================="

# ../build/PlaJA --engine FAULT_ANALYSIS \
#   --model-file "$MODEL" \
#   --seed 1234 \
#   --max-time 1800 \
#   --nn-interface "$NN_INTERFACE" \
#   --nn "$NN_FILE" \
#   --initial-state-enum inc \
#   --prop 1 \
#   --additional-properties "$ADD_PROPS" \
#   --oracle-use finding \
#   --oracle-mode infinite \
#   --infinite-oracle LRTDP \
#   --applicability-filtering 1 \
#   --fault-experiment-mode individualSearch \
#   --search-dir backward \
#   --seconds-per-query 60 \
#   --num-starts 1 \
#   --save-intermediate-stats \
#   --distance-func avoid \
#   --retraining-train-test-split 1.0 \
#   --rand-prob 1 \
#   --stats-file birds_logs/running_stats_flappy_birds_lrtdp_x${x}_y${y}_walls${num_walls}.csv \
#   --print-stats >> birds_logs/lrtdp_flap_x${x}_y${y}_walls${num_walls}.txt


echo "==============================================="
echo "All runs complete."
echo "==============================================="
