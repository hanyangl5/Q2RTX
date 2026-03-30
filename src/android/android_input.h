#pragma once

#include <stdbool.h>
#include <SDL2/SDL_events.h>

void Android_InputInit(void);
void Android_InputShutdown(void);
void Android_InputHandleTouch(const SDL_TouchFingerEvent *event, int width, int height);
void Android_InputHandleControllerAxis(const SDL_ControllerAxisEvent *event);
void Android_InputHandleControllerButton(const SDL_ControllerButtonEvent *event);
void Android_InputHandleControllerAdded(int which);
void Android_InputHandleControllerRemoved(SDL_JoystickID which);
void Android_InputOnPause(void);
bool Android_InputGetMotion(int *dx, int *dy);
void Android_InputDrawControls(int hud_width, int hud_height);
