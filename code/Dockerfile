# Stage 1: Build
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

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
        tzdata && \
    rm -rf /var/lib/apt/lists/*


RUN python3.8 -m ensurepip && python3.8 -m pip install --upgrade pip
RUN ln -sf /usr/bin/python3.8 /usr/bin/python3
RUN ln -s /usr/local/bin/pip3 /usr/bin/pip3

   
WORKDIR /workspace
# Copy project source (including gurobi)
COPY . /workspace/plaja

# Install Gurobi
WORKDIR /workspace/plaja/dependencies
RUN wget -v https://packages.gurobi.com/12.0/gurobi12.0.0_linux64.tar.gz \
&& tar -xvf gurobi12.0.0_linux64.tar.gz \
&& rm *.tar.gz && mv -f gurobi1200/ gurobi && mv -f gurobi/linux64/ gurobi/linux
WORKDIR /workspace/plaja/dependencies/gurobi/linux/src/build
RUN make && cp libgurobi_c++.a ../../lib

# Set GUROBI_HOME so cmake/make can find it
ENV GUROBI_HOME=/workspace/plaja/dependencies/gurobi/linux
ENV PATH=$GUROBI_HOME/bin:$PATH
ENV LD_LIBRARY_PATH=$GUROBI_HOME/lib
ENV PYBIND11_PYTHON_EXECUTABLE=/usr/bin/python3.8

# Build main project
WORKDIR /workspace/plaja/build
RUN rm -rf /workspace/plaja/dependencies/veritas/manual_build
RUN rm -rf /workspace/plaja/dependencies/Marabou/build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DGUROBI_DIR=/workspace/plaja/dependencies/gurobi/linux -DDOCKER_BUILD=ON .. && make -j5

# Stage 2: Runtime
FROM ubuntu:22.04

WORKDIR /workspace/plaja

# Copy everything
COPY --from=builder /usr/local /usr/local
COPY --from=builder /workspace/plaja /workspace/plaja

ENTRYPOINT ["./PlaJA"]
WORKDIR /workspace/plaja/build
CMD ["--engine", "FAULT_ANALYSIS", "--help"]
