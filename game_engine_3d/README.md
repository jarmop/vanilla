## Install dependencies

```
sudo apt install libglfw3-dev
sudo apt install libcglm-dev
```

## The GLAD loader
The code in the glad directory was generated using https://glad.dav1d.de/, like this:

Configure:
- Language: C/C++
- specification: OpenGL
- API: gl --> Version 3.3
- Profile: Core

Generate and download the zip file.

Extract the zip file and copy the "glad.c" and "glad.h" to the glad directory. Fix the "glad.h" include path in "glad.c".

## OpenGL 3.3 reference
https://registry.khronos.org/OpenGL/specs/gl/glspec33.core.pdf
