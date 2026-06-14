FROM fedora

RUN dnf -y update && \
    dnf config-manager disable fedora-cisco-openh264 && \
    dnf -y install --setopt=tsflags=nodocs --setopt=install_weak_deps=False \
    cmake \
    gcc \
    gcc-c++ \
    ninja-build \
    freetype-devel \
    libicu-devel \
    libstdc++-devel \
    mesa-libGL-devel \
    libglvnd-devel \
    qt6-qtbase-devel \
    qt6-qtpositioning \
    qt6-qtpositioning-devel \
    qt6-qtsvg-devel \
    sqlite-devel \

    # for GLFW X11 backend
    libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \

    boost-devel \
    ccache \
    git \
    bash-completion \
    # to stop build in VS Code
    procps-ng \
    && \
    dnf clean all && \
    rm -rf /var/cache/dnf/*

# needed for Docker
ARG USERNAME=dev
RUN useradd $USERNAME && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME
USER $USERNAME

# vim: autowrite
