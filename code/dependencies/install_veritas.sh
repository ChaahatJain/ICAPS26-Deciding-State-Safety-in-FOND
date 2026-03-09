#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [ ! -d $mydir/veritas ]
then
git clone https://github.com/laudv/veritas.git
fi

# clone this repository
cd veritas
git fetch --all
git checkout finiteprec
git submodule init && git submodule update

# editable install
pip3 install --editable .

# build manually to avoid scikit-learn issues
BUILD_DIR="manual_build"

# If it exists, delete it
if [ -d "$BUILD_DIR" ]; then
    echo "==> Removing existing $BUILD_DIR ..."
    rm -rf "$BUILD_DIR" || { echo "ERROR: Could not remove $BUILD_DIR"; exit 1; }
fi

# Now create it fresh
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# symlink to .so in src/python/veritas
cd ..
TARGET_FILE=$(ls ${BUILD_DIR}/veritas_core*.so 2> /dev/null)

# Check if a target file was found
if [ -z "$TARGET_FILE" ]; then
  echo "No veritas_core*.so file found in ${BUILD_DIR}"
  exit 1
fi

# Extract the filename from the target file path
TARGET_BASENAME=$(basename "$TARGET_FILE")

# Set the link name
LINK_NAME="src/python/veritas/${TARGET_BASENAME}"

# Determine the operating system
OS="$(uname)"

# Create symbolic link based on the OS
if [ "$OS" == "Darwin" ]; then
  # macOS: Use absolute path due to macOS limitation with relative symlinks
  ABS_TARGET=$(cd "$(dirname "$TARGET_FILE")" && pwd)/$(basename "$TARGET_FILE")
  ln -sf "$ABS_TARGET" "$LINK_NAME" || { echo "Failed to create symlink on macOS"; exit 1; }
else
  # Assume Linux or other Unix-like OS: Use relative path
  ln -sr "$TARGET_FILE" "$LINK_NAME" || { echo "Failed to create symlink on Linux"; exit 1; }
fi