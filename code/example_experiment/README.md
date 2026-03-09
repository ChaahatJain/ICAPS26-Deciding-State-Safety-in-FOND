# PlaJA Example Experiment

This repository contains the code and scripts required to build **PlaJA** and run the example experiments. The instructions below describe how to start the required Docker environment, build the project, and reproduce the example experiment results.

---

## 1. Start the Docker Environment

Run the following command from the root of this repository to start the Docker container:

```bash
docker run --rm -it \
  -v "$(pwd)/code:/ws" \
  -v "$(pwd)/benchmarks:/dataset" \
  -w /ws \
  chaahatjain/plaja_dependencies:MRv0.5 \
  /bin/bash
```

This command performs the following:

* Mounts the local `code/` directory to `/ws` inside the container (workspace).
* Mounts the local `benchmarks/` directory to `/dataset` inside the container.
* Sets `/ws` as the working directory.

After running the command, you will be inside the container shell.

---

## 2. Build PlaJA

Inside the container, build PlaJA using the following commands:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j5
```

This will compile the project in **release mode**.

---

## 3. Run the Example Experiments

Navigate to the example experiment directory:

```bash
cd example_experiment
```

### 3.1 Find All Faults on Unsafe Paths

Run the following script to identify faults that occur on unsafe paths:

```bash
./faults.sh 1000 10 250 950
```

### 3.2 Determine Safe States on Unsafe Paths

Run the following script to determine which states are safe along unsafe paths:

```bash
./state_safety.sh 1000 10 250 950
```

---

## 4. Results

The results of the experiments are automatically written to log files generated during execution.
These logs contain the output and statistics produced by the experiment scripts.

---

## Notes

* Ensure that the `code` and `benchmarks` directories exist in the repository root before running the Docker command.
* All commands after starting the container should be executed **inside the Docker environment**.
* The provided scripts assume the directory structure used in this repository.
