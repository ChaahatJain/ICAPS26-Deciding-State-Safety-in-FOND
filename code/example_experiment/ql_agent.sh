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
echo "Running QL_AGENT for x=${x}, y=${y}, walls=${num_walls} skip=${skip}"
echo "==============================================="

../build/PlaJA --engine QL_AGENT \
  --model-file "$MODEL" \
  --prop 1 \
  --additional-properties "$ADD_PROPS" \
  --nn-interface "$NN_INTERFACE" \
  --save-nnet \
  --initial-state-enum sample \
  --num-episodes 5000 \
  --max-length-episode 1100 \
  --print-stats \
  --applicability-filtering 1 \
  --max-time 43200

echo "==============================================="
echo "All runs complete."
echo "==============================================="
