//
// Main entry point for Dip emulator/interpreter
//
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "cpu.h"
#include "keypad.h"

int scale = 10;

SDL_AudioSpec have;
SDL_AudioDeviceID dev;

// Handles the updating of the screen output
void update_screen(SDL_Renderer* renderer) {
  // Clear the back buffer
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 250);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 250);

  // Update the screen buffer
  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      if (gfx[x + y * 64] == 1) {
        SDL_Rect rect = {
          .x = (x * scale),
          .y = (y * scale),
          .w = scale,
          .h = scale
        };
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }

  // Display update
  SDL_RenderPresent(renderer);
}

// Main function to load a ROM
size_t load_rom(uint8_t *buffer, char *rom_path) {
  FILE *fp = NULL;

  fp = fopen(rom_path, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Doesn't appear to be a file hombre. Try another one\n");
    exit(2);
  }

  // Get the size of the ROM
  fseek(fp, 0L, SEEK_END);
  int sz = ftell(fp);

  // Go back to the beginning
  rewind(fp);

  size_t read_bytes = fread(buffer, 1, sz, fp);

  // Close the ROM file
  fclose(fp);

  return read_bytes;
}

// Usage instructions for the emulator
int print_usage() {
  printf(
"Usage: dip -r [path_to_rom]\n\n"
"  -r [path_to_rom]       Load from from path\n");

  exit(EXIT_SUCCESS);
}

void init_audio() {

  SDL_AudioSpec want = {
    .freq = 12000,
    .format = AUDIO_F32LSB,
    .channels = 1,
    .samples = 64
  };

  dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);
  SDL_PauseAudioDevice(dev, 0);
}

// Create a tone and queue it up for playing
void update_sound(SDL_AudioDeviceID* dev, SDL_AudioSpec* have, uint8_t* sound_timer) {
  uint8_t sample[4];

  if (*sound_timer > 0) {
    uint8_t src[] = { 0x00, 0x00, 0x80, 0x3f };
    memcpy(sample, src, 4);
  }

  int n = have->channels * have->samples * 4;
  uint8_t *data = malloc(n);

  for (int i = 0; i < n; i += 4) {
    for (int j = 0; j < 4; j++) {
      data[i+j] = sample[j];
    }
  }

  SDL_QueueAudio(*dev, data, n);

  free(data);
}

int main(int argc, char **argv) {

  char rom_path[256];

  // Parse arguments
  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-r")) {
      if (i == argc-1) {
        print_usage();
      }
      strncpy(rom_path, argv[++i], sizeof(rom_path));
    } else if (i == argc-1) {
      // If we've run out of arguments to parse, print out the usage
      print_usage();
    }
  }

  printf("ROM location: %s\n", rom_path);
  // Load the ROM
  int bufferSize = 2048;
  uint8_t buffer[bufferSize];
  int rom_size = load_rom(buffer, rom_path);

  printf("ROM size: %d\n", rom_size);

  // Setup graphics and inputs
  // SDL2 bindings here
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;

  // Useful for simple alert dialog: https://wiki.libsdl.org/SDL_ShowSimpleMessageBox

  // Init SDL with Video and Audio enabled
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  // Create the window and renderer
  SDL_CreateWindowAndRenderer(640, 320, 0, &window, &renderer);

  SDL_SetWindowTitle(window, "Dip 🕹");

  init_audio();

  // Initialize the CPU / Memory etc
  initialize(buffer, bufferSize);

  uint32_t start_time = SDL_GetTicks();
  uint32_t current_time = 0;

  // Main game loop
  while(1) {
    SDL_Event e;

    // Handle input
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        printf("Exiting...\n");
        break;
      }
      handle_input(e);
    }

    // Emulate a cycle of the CPU
    // Every millisecond update the cpu
    emulate_cycle();

    current_time = SDL_GetTicks();
    if (current_time > start_time + 15) {
      // Should be 60Hz
      update_timers();

      update_sound(&dev, &have, &sound_timer);
      start_time = SDL_GetTicks();
    }

    // Handle screen update
    if (drawFlag) {
      update_screen(renderer);
      // Set back to 0
      drawFlag = 0;
    }

    // Add a delay
    SDL_Delay(1);
  }

  // Tear down SDL bindings
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
