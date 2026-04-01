/*
Copyright (C) 2026
*/

#include "shared/shared.h"
#include "common/cvar.h"
#include "../client/client.h"
#include "client/keys.h"
#include "client/ui.h"
#include "refresh/refresh.h"
#include "android_input.h"

#include <SDL2/SDL.h>
#include <string.h>

typedef enum {
    TOUCH_ROLE_NONE,
    TOUCH_ROLE_MOVE_UP,
    TOUCH_ROLE_MOVE_LEFT,
    TOUCH_ROLE_MOVE_DOWN,
    TOUCH_ROLE_MOVE_RIGHT,
    TOUCH_ROLE_LOOK,
    TOUCH_ROLE_ATTACK,
    TOUCH_ROLE_USE,
    TOUCH_ROLE_JUMP,
    TOUCH_ROLE_WEAPON_PREV,
    TOUCH_ROLE_WEAPON_NEXT,
    TOUCH_ROLE_MENU
} touch_role_t;

typedef struct {
    SDL_FingerID id;
    touch_role_t role;
    float anchor_x;
    float anchor_y;
    float x;
    float y;
} touch_slot_t;

typedef struct {
    unsigned key;
    bool down;
} android_key_state_t;

static cvar_t *android_touch_controls;
static cvar_t *android_touch_left_handed;
static cvar_t *android_touch_look_sensitivity;
static cvar_t *android_touch_deadzone;
static cvar_t *android_touch_alpha;
static cvar_t *android_gyro_enable;

static touch_slot_t touch_slots[8];
static SDL_GameController *controllers[4];
static android_key_state_t move_keys[] = {
    { 'w', false },
    { 'a', false },
    { 's', false },
    { 'd', false },
    { ' ', false },
    { 'e', false },
    { K_MOUSE1, false }
};

static int motion_x;
static int motion_y;
static Sint16 controller_move_x;
static Sint16 controller_move_y;
static Sint16 controller_look_x;
static Sint16 controller_look_y;

static unsigned android_time(void)
{
    return Sys_Milliseconds();
}

static void set_key_state(unsigned key, bool down)
{
    size_t i;

    for (i = 0; i < q_countof(move_keys); i++) {
        if (move_keys[i].key != key) {
            continue;
        }
        if (move_keys[i].down == down) {
            return;
        }
        move_keys[i].down = down;
        Key_Event(key, down, android_time());
        return;
    }

    Key_Event(key, down, android_time());
}

static void pulse_key(unsigned key)
{
    unsigned time = android_time();

    Key_Event(key, true, time);
    Key_Event(key, false, time);
}

static void clear_touch_keys(void)
{
    size_t i;

    for (i = 0; i < q_countof(move_keys); i++) {
        if (move_keys[i].down) {
            move_keys[i].down = false;
            Key_Event(move_keys[i].key, false, android_time());
        }
    }
}

static void reset_touch_slots(void)
{
    memset(touch_slots, 0, sizeof(touch_slots));
    clear_touch_keys();
    motion_x = 0;
    motion_y = 0;
}

static touch_slot_t *find_touch_slot(SDL_FingerID id)
{
    size_t i;

    for (i = 0; i < q_countof(touch_slots); i++) {
        if (touch_slots[i].role != TOUCH_ROLE_NONE && touch_slots[i].id == id) {
            return &touch_slots[i];
        }
    }

    return NULL;
}

static touch_slot_t *alloc_touch_slot(SDL_FingerID id)
{
    size_t i;

    for (i = 0; i < q_countof(touch_slots); i++) {
        if (touch_slots[i].role == TOUCH_ROLE_NONE) {
            memset(&touch_slots[i], 0, sizeof(touch_slots[i]));
            touch_slots[i].id = id;
            return &touch_slots[i];
        }
    }

    return NULL;
}

static void release_touch_slot(touch_slot_t *slot)
{
    if (!slot) {
        return;
    }

    switch (slot->role) {
    case TOUCH_ROLE_MOVE_UP:
        set_key_state('w', false);
        break;
    case TOUCH_ROLE_MOVE_LEFT:
        set_key_state('a', false);
        break;
    case TOUCH_ROLE_MOVE_DOWN:
        set_key_state('s', false);
        break;
    case TOUCH_ROLE_MOVE_RIGHT:
        set_key_state('d', false);
        break;
    case TOUCH_ROLE_ATTACK:
        set_key_state(K_MOUSE1, false);
        break;
    case TOUCH_ROLE_USE:
        set_key_state('e', false);
        break;
    case TOUCH_ROLE_JUMP:
        set_key_state(' ', false);
        break;
    case TOUCH_ROLE_LOOK:
    case TOUCH_ROLE_WEAPON_PREV:
    case TOUCH_ROLE_WEAPON_NEXT:
    case TOUCH_ROLE_MENU:
    case TOUCH_ROLE_NONE:
        break;
    }

    memset(slot, 0, sizeof(*slot));
}

static bool point_in_circle(float x, float y, float cx, float cy, float radius)
{
    float dx = x - cx;
    float dy = y - cy;
    return dx * dx + dy * dy <= radius * radius;
}

static bool is_menu_dest(void)
{
    return Key_GetDest() & KEY_MENU;
}

static bool is_touch_enabled(void)
{
    return android_touch_controls && android_touch_controls->integer != 0;
}

static bool is_move_role(touch_role_t role)
{
    return role == TOUCH_ROLE_MOVE_UP ||
        role == TOUCH_ROLE_MOVE_LEFT ||
        role == TOUCH_ROLE_MOVE_DOWN ||
        role == TOUCH_ROLE_MOVE_RIGHT;
}

static unsigned move_role_key(touch_role_t role)
{
    switch (role) {
    case TOUCH_ROLE_MOVE_UP:
        return 'w';
    case TOUCH_ROLE_MOVE_LEFT:
        return 'a';
    case TOUCH_ROLE_MOVE_DOWN:
        return 's';
    case TOUCH_ROLE_MOVE_RIGHT:
        return 'd';
    default:
        return 0;
    }
}

static float move_pad_center_x(void)
{
    return (android_touch_left_handed && android_touch_left_handed->integer != 0) ? 0.84f : 0.16f;
}

static touch_role_t classify_move_role(float x, float y)
{
    float center_x = move_pad_center_x();

    if (point_in_circle(x, y, center_x, 0.61f, 0.055f)) {
        return TOUCH_ROLE_MOVE_UP;
    }
    if (point_in_circle(x, y, center_x - 0.09f, 0.72f, 0.055f)) {
        return TOUCH_ROLE_MOVE_LEFT;
    }
    if (point_in_circle(x, y, center_x, 0.83f, 0.055f)) {
        return TOUCH_ROLE_MOVE_DOWN;
    }
    if (point_in_circle(x, y, center_x + 0.09f, 0.72f, 0.055f)) {
        return TOUCH_ROLE_MOVE_RIGHT;
    }

    return TOUCH_ROLE_NONE;
}

static void update_move_role(touch_slot_t *slot, float x, float y)
{
    touch_role_t new_role = classify_move_role(x, y);

    if (!is_move_role(slot->role)) {
        return;
    }

    if (new_role == slot->role) {
        return;
    }

    set_key_state(move_role_key(slot->role), false);

    if (is_move_role(new_role)) {
        slot->role = new_role;
        set_key_state(move_role_key(slot->role), true);
    }
}

static touch_role_t classify_role(float x, float y)
{
    bool left_handed = android_touch_left_handed && android_touch_left_handed->integer != 0;
    float action_center_x = left_handed ? 0.14f : 0.86f;
    float use_center_x = left_handed ? 0.26f : 0.74f;
    float jump_center_x = left_handed ? 0.08f : 0.92f;
    float menu_center_x = left_handed ? 0.06f : 0.94f;
    touch_role_t move_role = classify_move_role(x, y);

    if (move_role != TOUCH_ROLE_NONE) {
        return move_role;
    }

    if (point_in_circle(x, y, menu_center_x, 0.08f, 0.05f)) {
        return TOUCH_ROLE_MENU;
    }
    if (point_in_circle(x, y, action_center_x, 0.72f, 0.10f)) {
        return TOUCH_ROLE_ATTACK;
    }
    if (point_in_circle(x, y, use_center_x, 0.82f, 0.08f)) {
        return TOUCH_ROLE_USE;
    }
    if (point_in_circle(x, y, jump_center_x, 0.56f, 0.08f)) {
        return TOUCH_ROLE_JUMP;
    }
    if (point_in_circle(x, y, action_center_x, 0.20f, 0.06f)) {
        return TOUCH_ROLE_WEAPON_NEXT;
    }
    if (point_in_circle(x, y, use_center_x, 0.20f, 0.06f)) {
        return TOUCH_ROLE_WEAPON_PREV;
    }

    return TOUCH_ROLE_LOOK;
}

static void handle_game_touch_down(touch_slot_t *slot, float x, float y)
{
    slot->role = classify_role(x, y);
    slot->anchor_x = x;
    slot->anchor_y = y;
    slot->x = x;
    slot->y = y;

    switch (slot->role) {
    case TOUCH_ROLE_MOVE_UP:
    case TOUCH_ROLE_MOVE_LEFT:
    case TOUCH_ROLE_MOVE_DOWN:
    case TOUCH_ROLE_MOVE_RIGHT:
        set_key_state(move_role_key(slot->role), true);
        break;
    case TOUCH_ROLE_ATTACK:
        set_key_state(K_MOUSE1, true);
        break;
    case TOUCH_ROLE_USE:
        set_key_state('e', true);
        break;
    case TOUCH_ROLE_JUMP:
        set_key_state(' ', true);
        break;
    case TOUCH_ROLE_WEAPON_PREV:
        pulse_key(K_MWHEELDOWN);
        break;
    case TOUCH_ROLE_WEAPON_NEXT:
        pulse_key(K_MWHEELUP);
        break;
    case TOUCH_ROLE_MENU:
        pulse_key(K_ESCAPE);
        break;
    case TOUCH_ROLE_LOOK:
    case TOUCH_ROLE_NONE:
        break;
    }
}

static void handle_game_touch_motion(touch_slot_t *slot, float x, float y, int width, int height)
{
    float dx = x - slot->x;
    float dy = y - slot->y;

    slot->x = x;
    slot->y = y;

    switch (slot->role) {
    case TOUCH_ROLE_MOVE_UP:
    case TOUCH_ROLE_MOVE_LEFT:
    case TOUCH_ROLE_MOVE_DOWN:
    case TOUCH_ROLE_MOVE_RIGHT:
        update_move_role(slot, x, y);
        break;
    case TOUCH_ROLE_LOOK:
        motion_x += Q_rint(dx * width * android_touch_look_sensitivity->value);
        motion_y += Q_rint(dy * height * android_touch_look_sensitivity->value);
        break;
    default:
        break;
    }
}

static void draw_button(float cx, float cy, float radius, uint32_t color)
{
    int x = (int)((cx - radius) * r_config.width);
    int y = (int)((cy - radius) * r_config.height);
    int w = (int)(radius * 2.0f * r_config.width);
    int h = (int)(radius * 2.0f * r_config.height);

    R_DrawFill32(x, y, w, h, color);
}

void Android_InputInit(void)
{
    android_touch_controls = Cvar_Get("android_touch_controls", "1", CVAR_ARCHIVE);
    android_touch_left_handed = Cvar_Get("android_touch_left_handed", "0", CVAR_ARCHIVE);
    android_touch_look_sensitivity = Cvar_Get("android_touch_look_sensitivity", "2.5", CVAR_ARCHIVE);
    android_touch_deadzone = Cvar_Get("android_touch_deadzone", "0.12", CVAR_ARCHIVE);
    android_touch_alpha = Cvar_Get("android_touch_alpha", "0.28", CVAR_ARCHIVE);
    android_gyro_enable = Cvar_Get("android_gyro_enable", "0", CVAR_ARCHIVE);

    (void)android_gyro_enable;
    reset_touch_slots();
}

void Android_InputShutdown(void)
{
    size_t i;

    reset_touch_slots();
    for (i = 0; i < q_countof(controllers); i++) {
        if (controllers[i]) {
            SDL_GameControllerClose(controllers[i]);
            controllers[i] = NULL;
        }
    }
}

void Android_InputHandleTouch(const SDL_TouchFingerEvent *event, int width, int height)
{
    touch_slot_t *slot;
    int mouse_x = Q_rint(event->x * width);
    int mouse_y = Q_rint(event->y * height);

    if (!is_touch_enabled()) {
        return;
    }

    if (is_menu_dest()) {
        UI_MouseEvent(mouse_x, mouse_y);
        if (event->type == SDL_FINGERDOWN) {
            Key_Event(K_MOUSE1, true, event->timestamp);
        } else if (event->type == SDL_FINGERUP) {
            Key_Event(K_MOUSE1, false, event->timestamp);
        }
        return;
    }

    slot = find_touch_slot(event->fingerId);
    if (!slot && event->type != SDL_FINGERUP) {
        slot = alloc_touch_slot(event->fingerId);
    }
    if (!slot) {
        return;
    }

    if (event->type == SDL_FINGERDOWN) {
        handle_game_touch_down(slot, event->x, event->y);
        return;
    }

    if (event->type == SDL_FINGERMOTION) {
        handle_game_touch_motion(slot, event->x, event->y, width, height);
        return;
    }

    release_touch_slot(slot);
}

void Android_InputHandleControllerAxis(const SDL_ControllerAxisEvent *event)
{
    switch (event->axis) {
    case SDL_CONTROLLER_AXIS_LEFTX:
        controller_move_x = event->value;
        break;
    case SDL_CONTROLLER_AXIS_LEFTY:
        controller_move_y = event->value;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTX:
        controller_look_x = event->value;
        break;
    case SDL_CONTROLLER_AXIS_RIGHTY:
        controller_look_y = event->value;
        break;
    default:
        return;
    }

    set_key_state('a', controller_move_x < -12000);
    set_key_state('d', controller_move_x > 12000);
    set_key_state('w', controller_move_y < -12000);
    set_key_state('s', controller_move_y > 12000);
}

void Android_InputHandleControllerButton(const SDL_ControllerButtonEvent *event)
{
    bool down = event->state == SDL_PRESSED;

    switch (event->button) {
    case SDL_CONTROLLER_BUTTON_A:
        set_key_state(' ', down);
        break;
    case SDL_CONTROLLER_BUTTON_B:
        if (down) {
            pulse_key(K_ESCAPE);
        }
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        if (down) {
            pulse_key(K_MWHEELUP);
        }
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        if (down) {
            pulse_key(K_MWHEELDOWN);
        }
        break;
    case SDL_CONTROLLER_BUTTON_X:
        set_key_state('e', down);
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        set_key_state(K_MOUSE1, down);
        break;
    default:
        break;
    }
}

void Android_InputHandleControllerAdded(int which)
{
    size_t i;

    if (!SDL_IsGameController(which)) {
        return;
    }

    for (i = 0; i < q_countof(controllers); i++) {
        if (!controllers[i]) {
            controllers[i] = SDL_GameControllerOpen(which);
            break;
        }
    }
}

void Android_InputHandleControllerRemoved(SDL_JoystickID which)
{
    size_t i;

    for (i = 0; i < q_countof(controllers); i++) {
        SDL_Joystick *joystick;

        if (!controllers[i]) {
            continue;
        }

        joystick = SDL_GameControllerGetJoystick(controllers[i]);
        if (joystick && SDL_JoystickInstanceID(joystick) == which) {
            SDL_GameControllerClose(controllers[i]);
            controllers[i] = NULL;
        }
    }
}

void Android_InputOnPause(void)
{
    reset_touch_slots();
    Key_ClearStates();
}

bool Android_InputGetMotion(int *dx, int *dy)
{
    int new_dx = motion_x;
    int new_dy = motion_y;

    if (abs(controller_look_x) > 8000) {
        new_dx += controller_look_x / 4096;
    }
    if (abs(controller_look_y) > 8000) {
        new_dy += controller_look_y / 4096;
    }

    if (!new_dx && !new_dy) {
        return false;
    }

    motion_x = 0;
    motion_y = 0;
    *dx = new_dx;
    *dy = new_dy;
    return true;
}

void Android_InputDrawControls(int hud_width, int hud_height)
{
    float alpha;
    uint32_t color;
    bool left_handed;
    float action_center_x;
    float use_center_x;
    float jump_center_x;
    float menu_center_x;
    float move_center_x;

    if (!is_touch_enabled() || is_menu_dest() || cls.state != ca_active) {
        return;
    }

    alpha = Cvar_ClampValue(android_touch_alpha, 0.05f, 0.75f);
    left_handed = android_touch_left_handed->integer != 0;
    action_center_x = left_handed ? 0.14f : 0.86f;
    use_center_x = left_handed ? 0.26f : 0.74f;
    jump_center_x = left_handed ? 0.08f : 0.92f;
    menu_center_x = left_handed ? 0.06f : 0.94f;
    move_center_x = left_handed ? 0.84f : 0.16f;
    color = ((uint32_t)(alpha * 255.0f) << 24) | 0x00ffffff;

    (void)hud_width;
    (void)hud_height;

    draw_button(move_center_x, 0.61f, 0.055f, color);
    draw_button(move_center_x - 0.09f, 0.72f, 0.055f, color);
    draw_button(move_center_x, 0.83f, 0.055f, color);
    draw_button(move_center_x + 0.09f, 0.72f, 0.055f, color);
    draw_button(action_center_x, 0.72f, 0.10f, color);
    draw_button(use_center_x, 0.82f, 0.08f, color);
    draw_button(jump_center_x, 0.56f, 0.08f, color);
    draw_button(action_center_x, 0.20f, 0.06f, color);
    draw_button(use_center_x, 0.20f, 0.06f, color);
    draw_button(menu_center_x, 0.08f, 0.05f, color);
}
