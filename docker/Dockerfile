FROM espressif/idf:release-v5.2

# Install system dependencies required by QEMU if missing
RUN apt-get update && apt-get install -y \
    libgcrypt20 libglib2.0-0 libpixman-1-0 libsdl2-2.0-0 libslirp0 \
    && rm -rf /var/lib/apt/lists/*

# Install QEMU for xtensa using IDF tools
RUN . $IDF_PATH/export.sh && \
    python $IDF_PATH/tools/idf_tools.py install qemu-xtensa

WORKDIR /workspace
