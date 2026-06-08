#include "frontend/common.h"
#include "game_boy.h"
#include "macros.h"
#include "scheduler.h"
#include "util.h"
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float x, y, width, height;
} Rectangle;

static GLuint texture = 0;
static u32 *pixels = nullptr;

static GameBoy *g_gb = nullptr;
static Scheduler *g_sched = nullptr;

static double g_start_time = 0.0;

static bool running = true;

#define THING(i)                                              \
    ((u32)PALETTE_RGB[i][0] | ((u32)PALETTE_RGB[i][1] << 8) | \
     ((u32)PALETTE_RGB[i][2] << 16) | 0xFF000000)

static void build_pixels(const GameBoy *gb)
{

    static const u32 PALETTE[] = {
        THING(0),
        THING(1),
        THING(2),
        THING(3),
    };
    static_assert(ARRAY_LEN(PALETTE) == PALETTE_RGB_LEN);

    for (size_t y = 0; y < GB_LCD_HEIGHT; ++y) {
        for (size_t x = 0; x < GB_LCD_WIDTH; ++x) {
            u8 color = gb->scanout_buf[y][x];
            pixels[(y * GB_LCD_WIDTH) + x] = PALETTE[color];
        }
    }
}

static void update_texture(void)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GB_LCD_WIDTH, GB_LCD_HEIGHT,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static void draw_quad(FitRect dest, int screen_width, int screen_height)
{
    float left = ((dest.x / (float)screen_width) * 2) - 1;
    float right = (((dest.x + dest.w) / (float)screen_width) * 2) - 1;
    float top = 1 - ((dest.y / (float)screen_height) * 2);
    float bottom = 1 - (((dest.y + dest.h) / (float)screen_height) * 2);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(left, top);
    glTexCoord2f(1, 0);
    glVertex2f(right, top);
    glTexCoord2f(1, 1);
    glVertex2f(right, bottom);
    glTexCoord2f(0, 1);
    glVertex2f(left, bottom);
    glEnd();
}

static void display(void)
{
    long double frame_start = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    long double cur_time = frame_start - g_start_time;
    u64 cur_time_clk = (u64)(cur_time * GB_CLK_FREQ_HZ);

    sched_dispatch_until(&g_sched, g_gb, cur_time_clk);
    build_pixels(g_gb);
    update_texture();

    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);

    FitRect fit = fit_rect_to_ratio(0, 0, (float)width, (float)height,
                                    GB_LCD_ASPECT_RATIO);

    glViewport(0, 0, width, height);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texture);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    draw_quad(fit, width, height);

    glutSwapBuffers();
    glutPostRedisplay();
}

static bool *map_key_down(JoypadButtons *btns, unsigned char key)
{
    switch (key) {
        case 13:
            return &btns->start;
        case ' ':
            return &btns->select;
        case 'x':
            return &btns->a;
        case 'z':
            return &btns->b;
        default:
            return nullptr;
    }
}

static void key_down(unsigned char key, [[maybe_unused]] int x,
                     [[maybe_unused]] int y)
{
    if (key == 27) {
        running = false;
        glutLeaveMainLoop();
        return;
    }

    bool *btn = map_key_down(&g_gb->btns, key);

    if (btn != nullptr)
        *btn = true;
}

static void key_up(unsigned char key, [[maybe_unused]] int x,
                   [[maybe_unused]] int y)
{
    bool *btn = map_key_down(&g_gb->btns, key);

    if (btn != nullptr)
        *btn = false;
}

static bool *map_special_key(JoypadButtons *btns, int key)
{
    switch (key) {
        case GLUT_KEY_UP:
            return &btns->up;
        case GLUT_KEY_DOWN:
            return &btns->down;
        case GLUT_KEY_LEFT:
            return &btns->left;
        case GLUT_KEY_RIGHT:
            return &btns->right;
        default:
            return nullptr;
    }
}

static void special_down(int key, [[maybe_unused]] int x,
                         [[maybe_unused]] int y)
{
    bool *btn = map_special_key(&g_gb->btns, key);

    if (btn != nullptr)
        *btn = true;
}

static void special_up(int key, [[maybe_unused]] int x, [[maybe_unused]] int y)
{
    bool *btn = map_special_key(&g_gb->btns, key);

    if (btn != nullptr)
        *btn = false;
}

static void init_gl(void)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GB_LCD_WIDTH, GB_LCD_HEIGHT, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

static int run(GameBoy *gb, Scheduler *sched)
{
    g_gb = gb;
    pixels = calloc((size_t)GB_LCD_WIDTH * GB_LCD_HEIGHT, sizeof(*pixels));
    assert(pixels != NULL);

    g_sched = sched;

    int argc = 1;

    char *argv[] = {
        "gemu",
        nullptr,
    };

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT);
    glutCreateWindow("gemu");

    init_gl();
    glutDisplayFunc(display);
    glutKeyboardFunc(key_down);
    glutKeyboardUpFunc(key_up);
    glutSpecialFunc(special_down);
    glutSpecialUpFunc(special_up);

    g_start_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

    glutMainLoop();
    glDeleteTextures(1, &texture);
    free(pixels);

    return EXIT_SUCCESS;
}

const Frontend selected_frontend = {
    .name = "opengl",
    .run = run,
};
