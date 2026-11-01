FROM fedora

RUN dnf config-manager disable fedora-cisco-openh264 && \
    dnf -y update && \
    dnf -y install --setopt=tsflags=nodocs --setopt=install_weak_deps=False \
    cmake \
    gcc \
    gcc-c++ \
    ninja-build \
    libstdc++-devel \
    mesa-libGL-devel \
    libglvnd-devel \
    qt6-qtbase-devel \
    qt6-qtpositioning-devel \
    qt6-qtsvg-devel \
    sqlite-devel \
    # for GLFW X11 backend
    libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
    # for WITH_SYSTEM_PROVIDED_3PARTY=ON
    gflags-devel expat-devel pugixml-devel utf8cpp-devel gtest-devel fast_float-devel freetype-devel boost-devel \
    #
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
