cmake_minimum_required(VERSION 3.25.1)
project(Project_1_2D_Platformer)

add_library(project_1_2d_platformer ./engine_from_scratch/src/glad.c 
./engine_from_scratch/src/main.c 
./engine_from_scratch/src/engine/global.c 
./engine_from_scratch/src/engine/render/render.c 
./engine_from_scratch/src/engine/render/render_init.c 
./engine_from_scratch/src/engine/render/render_util.c 
./engine_from_scratch/src/engine/io/io.c 
./engine_from_scratch/src/engine/time/time.c 
./engine_from_scratch/src/engine/config/config.c
./engine_from_scratch/src/engine/input/input.c)

include_directories(./include)

add_executable(main ./engine_from_scratch/src/main.c)

# link the 'my_program_code' library into the main executable
target_link_libraries(main project_1_2d_platformer SDL2 SDL2main freetype)