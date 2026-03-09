FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# --------------------------------------------------------------------------
# System packages + Python 3.8
# --------------------------------------------------------------------------

RUN apt-get update && apt-get install -y software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        git \
        python3.8 \
        python3.8-venv \
        python3.8-dev \
        python3.8-distutils \
        wget \
        curl \
        tzdata \
        unzip && \
    rm -rf /var/lib/apt/lists/*

# Make python3.8 default
RUN python3.8 -m ensurepip && \
    python3.8 -m pip install --upgrade pip && \
    ln -sf /usr/bin/python3.8 /usr/bin/python3 && \
    ln -sf /usr/bin/pip3.8 /usr/bin/pip3

# --------------------------------------------------------------------------
# Workspace
# --------------------------------------------------------------------------
RUN mkdir -p /workspace && chmod -R 777 /workspace
WORKDIR /workspace
COPY . /workspace/dependencies

# --------------------------------------------------------
# Install Lab
# -------------------------------------------------------
WORKDIR /workspace/dependencies
RUN unzip lab-master.zip && cd lab-master && pip install --editable . --break-system-packages && cd ..

# --------------------------------------------------------------------------
# Install model repair dependencies
# --------------------------------------------------------------------------
RUN pip3 install -r model_repair.txt

# --------------------------------------------------------------------------
# Install Gurobi
# --------------------------------------------------------------------------
WORKDIR /workspace/dependencies
RUN wget -v https://packages.gurobi.com/12.0/gurobi12.0.0_linux64.tar.gz \
 && tar -xvf gurobi12.0.0_linux64.tar.gz \
 && rm *.tar.gz \
 && mv -f gurobi1200/ gurobi \
 && mv -f gurobi/linux64/ gurobi/linux

WORKDIR /workspace/dependencies/gurobi/linux/src/build
RUN make -j$(nproc) && cp libgurobi_c++.a ../../lib

# Environment for Gurobi
ENV GUROBI_HOME=/workspace/dependencies/gurobi/linux
ENV PATH=$GUROBI_HOME/bin:$PATH
ENV LD_LIBRARY_PATH=$GUROBI_HOME/lib
ENV PYBIND11_PYTHON_EXECUTABLE=/usr/bin/python3.8

# --------------------------------------------------------------------------
# Build Z3
# --------------------------------------------------------------------------
WORKDIR /workspace/dependencies/z3
RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DZ3_SINGLE_THREADED=True -DPYTHON_EXECUTABLE=/usr/bin/python3.8 .. && \
    make -j$(nproc) && make install

# --------------------------------------------------------------------------
# Install other dependencies (your custom scripts)
# --------------------------------------------------------------------------
WORKDIR /workspace/dependencies
RUN set -e && \
    chmod +x install_veritas.sh install_marabou.sh install_torch.sh install_nlohmann.sh && \
    ./install_veritas.sh && \
    ./install_marabou.sh /workspace/dependencies/gurobi/linux && \
    ./install_torch.sh && \
    ./install_nlohmann.sh

# --------------------------------------------------------------------------
# Runtime setup
# --------------------------------------------------------------------------
WORKDIR /workspace
ENV PATH=/usr/local/bin:$PATH
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
ENV PYTHONPATH=/workspace/dependencies/veritas:/workspace/dependencies/Marabou

USER root

CMD ["/bin/bash"]
