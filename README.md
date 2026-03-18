# Algorithms for Deciding the Safety of States in Fully Observable Non-deterministic Problems — Camera-Ready Release

## Repository Structure
data and benchmarks are hosted on Zenodo: https://doi.org/10.5281/zenodo.18926835
```
.
├── benchmarks/         # Benchmark suite
│   ├── archive/        # Archived benchmark versions used in experiments
│   └── generator/      # Scripts to generate new benchmarks and add them to the suite
│
├── code/               # C++ codebase (must be built; see Build Instructions below)
│   └── example_experiment/   # End-to-end example of running fault analysis
│
└── data/               # Archived experimental data
    ├── logs/           # Fault analysis and oracle logs across different algorithms
    └── plotting/       # Plotting script to reproduce paper figures
```

---

## Build Instructions

The codebase is written in **C++** and requires a specific Docker image for its dependencies.

### Prerequisites

- [Docker](https://www.docker.com/) installed and running

### Step 1 — Pull the dependency image

```bash
docker pull chaahatjain/plaja_dependencies:MRv0.5 
```

> **Docker Hub:** https://hub.docker.com/repository/docker/chaahatjain/plaja_dependencies/tags/MRv0.5

### Step 2 — Build the code

Run the build inside the container, mounting the `code/` directory:

```bash
docker run --rm \
  -v $(pwd)/code:/ws \
  -w /ws \
  chaahatjain/plaja_dependencies:MRv0.5 /bin/bash
```

Follow this with
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 4
```
---

## Running an Experiment

The `code/example_experiment/` directory provides a complete, self-contained example demonstrating how to invoke the codebase for fault analysis. Refer to the scripts within that subdirectory for step-by-step instructions.

---

## Benchmarks

- **`benchmarks/`** — The benchmark suite used for all experiments in the paper.
- **`benchmarks/archive/`** — Frozen benchmark versions referenced in the evaluation of the paper.
- **`benchmarks/generator/`** — Utilities to generate new benchmarks and register them in the suite.

---

## Data & Benchmarks

Large files are hosted on Zenodo: https://doi.org/10.5281/zenodo.18926835

To set up locally please download and unzip into `data/` and `benchmarks/`

Alternatively, the following commands should work.
```
wget https://zenodo.org/record/18926835/files/data.zip
unzip data.zip
wget https://zenodo.org/record/18926835/files/benchmarks.zip
unzip benchmarks.zip
```
---

## Citation

If you use this artifact, please cite our paper:

```bibtex
@inproceedings{schmalz:jain:icaps26,
  title     = {Algorithms for Deciding the Safety of States in Fully Observable Non-deterministic Problems},
  author    = {Johannes Schmalz and Chaahat Jain},
  booktitle = {Proceedings of The 36th International Conference on Automated Planning and Scheduling (ICAPS)},
  year      = {2026}
}
```

---
