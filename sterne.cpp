#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdlib.h>
#include <vector>
#include <SDL.h>
#include <SDL_opengles2.h>
#ifdef __EMSCRIPTEN__
#include "imgui/examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif


typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat z;
} Coord;

GLfloat rnd() {
    return (GLfloat)rand() / (GLfloat)RAND_MAX;
}

void appendStars(std::vector<Coord> &coord, int n, GLfloat r, GLfloat m) {
    for (int i=0; i<n; i++)
        coord.push_back((Coord){r*(2.0f*rnd()-1.0f), r*(2.0f*rnd()-1.0f), m*rnd()});
}

void moveStars(std::vector<Coord> &coord, GLfloat m, GLfloat d) {
    for (Coord &c : coord) {
        c.z+=d;
        while (c.z>m)
            c.z-=m;
        while (c.z<0.0f)
            c.z+=m;
    }
}

const char* vertex_shader =
"#version 100\n"
"attribute vec3 coord;"
"varying lowp float vBright;"
"void main() {"
"  gl_Position = vec4(coord.x, coord.y, -0.1*coord.z, 2.0*coord.z-1.0);"
"  vBright = 1.2-(coord.z/4.5);"
"}";

const char* fragment_shader =
"#version 100\n"
"varying lowp float vBright;"
"void main() {"
"  gl_FragColor = vec4(vBright, vBright, vBright, 1.0);"
"}";

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window* window = SDL_CreateWindow("Sterne", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 100");

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs);
    glLinkProgram(shader_programme);

    int n_stars = 8000;
    const GLfloat d = 10.0f;
    std::vector<Coord> stars;
    GLfloat speed = -0.1f;

    bool done = false;
    Uint32 then = 0;
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        Uint32 now = SDL_GetTicks();
        double deltaTime = (now - then)/1000.0;
        then = now;

        if (n_stars>stars.size())
            appendStars(stars, n_stars-stars.size(), 6.0f, d);
        if (n_stars<stars.size())
            stars.resize(n_stars);

        std::vector<Coord> vertices;
        for (Coord c : stars) {
            vertices.push_back(c);
            c.z-=speed;
            vertices.push_back(c);
        }

        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Coord), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glUseProgram(shader_programme);
        GLint loc_speed = glGetUniformLocation(shader_programme, "speed");
        glUniform1f(loc_speed, speed);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDrawArrays(GL_LINES, 0, vertices.size());
        glDisableVertexAttribArray(0);
        moveStars(stars, d, speed*deltaTime/0.2);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                done = true;
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Settings");
        ImGui::SliderFloat("speed", &speed, -1.0f, 1.0f);
        ImGui::SliderInt("number of stars", &n_stars, 100, 40000);
        ImGui::Text("deltaTime %.3f (%.1f FPS)", deltaTime, io.Framerate);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
