#include <SDL2/SDL.h>

#include "cpu.h"

// Mapping of keys from SDL to our indices
uint8_t key_map[] = {
  SDLK_x, SDLK_1, SDLK_2, SDLK_3, // row 1...row n
  SDLK_q, SDLK_w, SDLK_e, SDLK_a,
  SDLK_s, SDLK_d, SDLK_z, SDLK_c,
  SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

// Runs a lookup over the keys that are pressed
int lookup_key(SDL_Keycode key) {
  for (int i = 0; i < 16; i++) {
    if (key_map[i] == key) {
      return i;
    }
  }
  return -1;
}

// Handle any input events
void handle_input(SDL_Event e) {
  int k;
  switch(e.type) {
    case SDL_KEYUP:
      k = lookup_key(e.key.keysym.sym);
      key[k] = 0;
      break;
    case SDL_KEYDOWN:
      k = lookup_key(e.key.keysym.sym);
      key[k] = 1;
      break;
  }
}