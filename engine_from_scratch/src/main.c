#include <stdio.h>
#include <stdbool.h>
#include </home/james/Development/opengl_projects/2d_platformer1/include/glad/glad.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "engine/global.h"

int main(int argc, char *argv[]) {
    render_init();
    
    bool should_quit = false;

    while (!should_quit) {

        glViewport(0,0,640,480);

        SDL_Event event;
        // Start event loop
        while (SDL_PollEvent(&event)) {
            // Handle each specific event
            // if (event.type == SDL_MOUSEMOTION) {
                // printf("mouse has been moved\n");
            // }
            if (event.type == SDL_KEYDOWN) {
                printf("a key has been pressed\n");
                if (event.key.keysym.sym == SDLK_0) {
                    printf("0 was pressed\n");
                } 
            }
            switch (event.type) {
                case SDL_QUIT:
                    should_quit = true;
                    break;
                default:
                    break;
            }
        }

        render_begin();

        render_quad(
            (vec2) {global.render.width * 0.5, global.render.height * 0.5},
            (vec2) {50, 50},
            (vec4) {0, 1, 0, 1});
        
        render_end();
    }
    return 0;
}