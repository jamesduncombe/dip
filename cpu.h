//
//
//
#ifndef CPU_H
#define CPU_H

void initialize();
void emulate_cycle();
void update_timers();

extern uint8_t gfx[64 * 32];

extern int drawFlag;

extern uint8_t sound_timer;

extern uint8_t key[16];

// Registers
// CHIP-8 has 16 8-bit registers
enum registers {
  V0,
  V1,
  V2,
  V3,
  V4,
  V5,
  V6,
  V7,
  V8,
  V9,
  VA,
  VB,
  VC,
  VD,
  VE,
  VF,
};

#endif // #CPU_H