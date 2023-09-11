# Start from a basic Arch Linux image
FROM archlinux:base-20210711.0.28748

# Some arguments
ARG USER_ID
ARG GROUP_ID

# Clean cache
# RUN pacman -Scc --noconfirm
# Disable signature check
RUN echo -e "[options]\nSigLevel = Never" >> /etc/pacman.conf
# Change database
RUN echo "Server=https://archive.archlinux.org/repos/2021/07/11/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist; \
    pacman --noconfirm -Syyuu
# Install some stuff
# RUN pacman -S --noconfirm python python-pip
RUN pacman -S --noconfirm base-devel vim git clang openmp cmake wget

# Create a new user called "benchmarker" (the default user)
# Add it to the wheel group
RUN groupadd -g ${GROUP_ID} benchmarker &&\
    useradd -m -u ${USER_ID} -g benchmarker -G wheel benchmarker
# Add password
RUN echo "benchmarker:password" | chpasswd
# Allow users in the wheel group to run commands with sudo
RUN echo '%wheel ALL=(ALL) ALL' >> /etc/sudoers

USER benchmarker

WORKDIR /home/benchmarker

# ADD https://api.github.com/repos/IlariaPilo/hashing-benchmark-docker/git/refs/heads/main version.json
# RUN git clone --recurse-submodules  https://github.com/IlariaPilo/hashing-benchmark-docker
# Install python dependencies
# RUN cd hashing-benchmark-docker && \
#    pip install --no-cache-dir -r requirements.txt

WORKDIR /home/benchmarker/hashing-benchmark-docker

# Run a shell as first operation
CMD ["/bin/bash"]