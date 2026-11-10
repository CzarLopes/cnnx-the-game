# Ambiente de desenvolvimento

Comandos para instalar o que o projeto precisa no Windows e no Linux. Em qualquer sistema, a primeira configuracao do CMake precisa de internet, porque baixa a raylib e a nlohmann_json.

## Windows

Use o `winget` no PowerShell. Ao terminar, abra um terminal novo para o PATH ser atualizado.

### Obrigatorios

```powershell
# CMake 3.20 ou mais novo
winget install --id Kitware.CMake -e

# MinGW-w64 com GCC 16.2 e mingw32-make (WinLibs, POSIX threads, UCRT)
winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e
```

### Opcionais

```powershell
# LDtk, para editar assets/levels/cnnx.ldtk
winget install --id deepnight.LDtk -e

# Python 3, para gerar o sprite do jogador (o script so usa a biblioteca padrao)
winget install --id Python.Python.3.13 -e
```

### Conferir, compilar e rodar

```powershell
cmake --version
gcc --version
mingw32-make --version

cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j 8
.\build\cnnx.exe
ctest --test-dir build --output-on-failure
```

## Linux

Alem do compilador e do CMake, a raylib precisa dos pacotes de desenvolvimento de X11, OpenGL e audio.

### Debian e Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake git python3 \
    libasound2-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev \
    libxinerama-dev libgl1-mesa-dev libglu1-mesa-dev \
    libwayland-dev libxkbcommon-dev
```

### Fedora

```bash
sudo dnf install gcc-c++ make cmake git python3 \
    alsa-lib-devel mesa-libGL-devel libX11-devel libXrandr-devel \
    libXi-devel libXcursor-devel libXinerama-devel libatomic \
    wayland-devel libxkbcommon-devel
```

### Arch

```bash
sudo pacman -S --needed base-devel cmake git python \
    alsa-lib mesa libx11 libxrandr libxi libxcursor libxinerama \
    wayland libxkbcommon
```

O LDtk para Linux pode ser baixado em <https://ldtk.io/>.

### Conferir, compilar e rodar

```bash
cmake --version
g++ --version

cmake -S . -B build
cmake --build build -j 8
./build/cnnx
ctest --test-dir build --output-on-failure
```

No Linux nao existe `%APPDATA%`, entao o `save.json` fica na pasta do executavel (`build/`).
