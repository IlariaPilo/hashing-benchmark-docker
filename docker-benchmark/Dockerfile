# Start from a basic Arch Linux image
FROM archlinux:base-20210711.0.28748

# Disable signature check
RUN echo -e "[options]\nSigLevel = Never" >> /etc/pacman.conf
# Change database
RUN echo "Server=https://archive.archlinux.org/repos/2021/07/11/\$repo/os/\$arch" > /etc//pacman.d/mirrorlist && \
    pacman --noconfirm -Syyuu
# Install some stuff
RUN pacman -S --noconfirm base-devel vim git python clang openmp cmake wget python-pip

# Create a new user called "benchmarker" (the default user)
# Add it to the wheel group
RUN useradd -m -G wheel benchmarker
# Add password
RUN echo "benchmarker:password" | chpasswd
# Allow users in the wheel group to run commands with sudo
RUN echo '%wheel ALL=(ALL) ALL' >> /etc/sudoers

USER benchmarker

WORKDIR /home/benchmarker

# Download the clone script
RUN wget -O clone.sh https://raw.githubusercontent.com/IlariaPilo/hashing-benchmark-docker/main/clone.sh
# Run the clone script
RUN sh clone.sh

# Install python dependencies
RUN cd hashing-benchmark-docker && \
    pip install --no-cache-dir -r requirements.txt

# Run a shell as first operation
CMD ["/bin/bash"]