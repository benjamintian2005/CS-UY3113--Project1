/**
* Author: Benjamin Tian
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 2.0f;

constexpr int WINDOW_WIDTH  = 640 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL    = 0;
constexpr GLint TEXTURE_BORDER     = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char BEAKER_SPRITE_FILEPATH[] = "scared.png",
               DROPS_SPRITE_FILEPATH[]  = "scared2.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE_DROPS  = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_DROPS    = glm::vec3(-1.0f, 1.0f, 0.0f),
                    INIT_SCALE_BEAKER = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_BEAKER   = glm::vec3(-1.0f, 0.0f, 0.0f);

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_beaker_matrix, g_projection_matrix, g_drops_matrix;

float g_previous_ticks = 0.0f;

GLuint g_beaker_texture_id;
GLuint g_drops_texture_id;

constexpr float DROPS_SPEED = 1.0f; // speed of drop
constexpr float BEAKER_SPEED = 1.0f; // speed of drop


glm::vec3 g_beaker_position   = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_beaker_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_drops_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_drops_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_drops_scale = glm::vec3(0.0f, 0.0f, 0.0f);  // scale trigger vector
glm::vec3 g_drops_size  = glm::vec3(1.0f, 1.0f, 0.0f);  // scale accumulator vector

void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("User-Input and Collisions Exercise",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_beaker_matrix = glm::mat4(1.0f);
    g_drops_matrix = glm::mat4(1.0f);
    g_drops_matrix = glm::translate(g_drops_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_drops_position += g_drops_movement;

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_beaker_texture_id = load_texture(BEAKER_SPRITE_FILEPATH);
    g_drops_texture_id = load_texture(DROPS_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q: g_app_status = TERMINATED; break;
                    default: break;
                }

            default:
                break;
        }
    }


    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (glm::length(g_drops_movement) > 1.0f)
    {
        // In this particular case, I'm not going to normalise because I am simulating
        // gravity
        // g_drops_movement = glm::normalize(g_drops_movement);
    }
}

void update()
{
    // --- DELTA TIME CALCULATIONS --- //
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    // --- UPDATE TIME FOR ANIMATIONS --- //
    static float g_time = 0.0f;
    g_time += delta_time;
    


    // --- Luffy Circular MOVEMENT  //
    float beaker_oscillation_amplitude = 1.0f; // how far the beaker moves
    float beaker_oscillation_speed = 1.5f; //  speed of movement
    g_beaker_movement.x = beaker_oscillation_amplitude * sin(beaker_oscillation_speed * g_time);
    g_beaker_movement.y = beaker_oscillation_amplitude * cos(beaker_oscillation_speed * g_time);


    // --- Eneru MOVEMENT (relative to luffy's movement) //
    
    g_drops_movement.x = ( 1.5f * g_beaker_movement.x ); //change of movement
    g_drops_movement.y = ( 0.5f * sin( g_beaker_movement.y) ); //change of movement

    g_drops_position.y = g_beaker_position.y + 0.5f; //relative to position
    
    
    // --- ACCUMULATOR LOGIC --- //
    g_drops_position += g_drops_movement * DROPS_SPEED * delta_time;
    g_drops_size += g_drops_scale * DROPS_SPEED * delta_time;
    
    g_beaker_position += g_beaker_movement * BEAKER_SPEED * delta_time;


    // --- OSCILLATING SCALE FOR Luffy --- //
    float scale_oscillation_speed = 2.0f;
    float scale_factor = sin(scale_oscillation_speed * g_time);

    // --- Luffy TRANSFORM --- //
    g_beaker_matrix = glm::mat4(1.0f);
    g_beaker_matrix = glm::translate(g_beaker_matrix, INIT_POS_BEAKER);
    g_beaker_matrix = glm::translate(g_beaker_matrix, g_beaker_position);

    g_beaker_matrix = glm::rotate(g_beaker_matrix, g_time * 1.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotating beaker
    g_beaker_matrix = glm::scale(g_beaker_matrix, INIT_SCALE_BEAKER);

    // --- Eneru TRANSFORM --- //
    g_drops_matrix = glm::mat4(1.0f);
    g_drops_matrix = glm::translate(g_drops_matrix, INIT_POS_DROPS);
    g_drops_matrix = glm::translate(g_drops_matrix, g_drops_position);

    //g_drops_matrix = glm::scale(g_drops_matrix, INIT_SCALE_DROPS);
    g_drops_matrix = glm::scale(g_drops_matrix, glm::vec3(scale_factor));

}



void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_drops_matrix, g_drops_texture_id);
    draw_object(g_beaker_matrix, g_beaker_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

