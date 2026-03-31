#include <stdio.h>
#include <stdlib.h>

#include "../glad/glad.h"

GLuint compile_shader(GLenum shaderType, int shdrSrcLen, const char *shdrSrc[]) {
    // Compile the shader
    unsigned int shdr;
    shdr = glCreateShader(shaderType);
    glShaderSource(shdr, shdrSrcLen, shdrSrc, NULL);
    glCompileShader(shdr);

    // Check shader compilation errors
    int success;
    glGetShaderiv(shdr, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(shdr, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation failed\n%s\n", infoLog);
        return 0;
    }

    return shdr;
}


char *read_file(const char *path) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *str = malloc(fsize + 1);
    if (fread(str, fsize, 1, f) != 1) {
        perror(path);
        exit(1);
    }
    fclose(f);

    str[fsize] = 0;

    return str;
}


GLuint get_shader_program(const char *vertPath, const char *fragPath) {
    // Compile the vertex shader
    const int vertShdrSrcLen = 1;
    const char *vertShdrSrc[1] = { read_file(vertPath) };
    GLuint vertShdr = compile_shader(GL_VERTEX_SHADER, vertShdrSrcLen, vertShdrSrc);
    free((void *)vertShdrSrc[0]);

    // Compile the fragment shader
    const int fragShdrSrcLen = 1;
    const char *fragShdrSrc[1] = { read_file(fragPath) };
    GLuint fragShdr = compile_shader(GL_FRAGMENT_SHADER, fragShdrSrcLen, fragShdrSrc);
    free((void *)fragShdrSrc[0]);

    // Link the shaders into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertShdr);
    glAttachShader(shaderProgram, fragShdr);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertShdr);
    glDeleteShader(fragShdr);

    // Check shader program linking errors
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "Shader program linking failed\n%s\n", infoLog);
        return 1;
    }

    return shaderProgram;
}