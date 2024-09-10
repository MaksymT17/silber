# Use the official Windows Server Core image as the base
FROM mcr.microsoft.com/windows/servercore:ltsc2022

# Install Chocolatey (a package manager for Windows)
RUN powershell -NoProfile -Command \
    Set-ExecutionPolicy Bypass -Scope Process -Force; \
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager+SecurityProtocolType]::Tls12; \
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))

# Install dependencies
RUN choco install -y \
    git \
    cmake \
    visualstudio2019buildtools \
    visualstudio2019-workload-vctools \
    visualstudio2019-workload-universal

# Set up environment variables for MSBuild
ENV PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin;${PATH}"

# Install Google Test
RUN git clone https://github.com/google/googletest.git C:\googletest && \
    mkdir C:\googletest\build && \
    cd C:\googletest\build && \
    cmake .. && \
    cmake --build . --config Release && \
    cmake --install . --config Release

# Set the working directory
WORKDIR /workspace

# Default command
CMD ["cmd.exe"]
