Install SDL and Slang as vendors:

```sh
mkdir vendored
cd vendored
git clone --depth=1 --branch=main https://github.com/libsdl-org/SDL.git
rm -rf SDL/.git
wget https://github.com/shader-slang/slang/releases/download/v2026.5.2/slang-2026.5.2-linux-x86_64.tar.gz
tar -xvf slang-2026.5.2-linux-x86_64.tar.gz slang
```

Build:

```sh
cmake -B build -G "Unix Makefiles"
cmake --build build
```
