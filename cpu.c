/*

Handles the CPU architecture of the CHIP-8

Includes all instructions

*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// Flag for whether to update the graphics output or not
drawFlag = 0;

// Registers
// CHIP-8 has 16 8-bit registers
// 16-bit index register
// 16-bit program counter
typedef struct {
  uint8_t V0;
  uint8_t V1;
  uint8_t V2;
  uint8_t V3;
  uint8_t V4;
  uint8_t V5;
  uint8_t V6;
  uint8_t V7;
  uint8_t V8;
  uint8_t V9;
  uint8_t VA;
  uint8_t VB;
  uint8_t VC;
  uint8_t VD;
  uint8_t VE;
  uint8_t VF;
  uint16_t I;
  uint16_t PC;
} Registers;

// Initialize the registers
Registers registers = {
  .V0 = 0,
  .V1 = 0,
  .V2 = 0,
  .V3 = 0,
  .V4 = 0,
  .V5 = 0,
  .V6 = 0,
  .V7 = 0,
  .V8 = 0,
  .V9 = 0,
  .VA = 0,
  .VB = 0,
  .VC = 0,
  .VD = 0,
  .VE = 0,
  .VF = 0,
  .I = 0,
  .PC = 0x200 // Program counter starts at 0x200
};

// Stack setup
uint8_t stack[16] = {0};
// Stack pointer
uint8_t SP = 0;

// Memory
// CHIP-8 has 4k of main memory
uint8_t memory[4096] = {0};

// Current opcode
uint16_t opcode = 0;

// Graphics
// Screen has a total of 2048 (64 * 32) pixels
uint8_t gfx[64 * 32];

// Interrupts and timers
// CHIP-8 has no interrupts but does have 2 timers
uint8_t delay_timer;
uint8_t sound_timer;

// Keyboard control
// CHIP-8 has total of 16 keys
uint8_t key[16];

// Helpers

uint8_t get_vreg(uint8_t vreg) {
  switch(vreg) {
    case 0x0:
      return registers.V0;
    case 0x1:
      return registers.V1;
    case 0x2:
      return registers.V2;
    case 0x3:
      return registers.V3;
    case 0x4:
      return registers.V4;
    case 0x5:
      return registers.V5;
    case 0x6:
      return registers.V6;
    case 0x7:
      return registers.V7;
    case 0x8:
      return registers.V8;
    case 0x9:
      return registers.V9;
    case 0xA:
      return registers.VA;
    case 0xB:
      return registers.VB;
    case 0xC:
      return registers.VC;
    case 0xD:
      return registers.VD;
    case 0xE:
      return registers.VE;
    case 0xF:
      return registers.VF;
  }
}

// Fontset from: http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
uint8_t chip8_fontset[80] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


// Instructions - opcode order

// 2nnn - CALL addr
// Call subroutine at nnn.
// The interpreter increments the stack pointer, then puts the current PC on
// the top of the stack. The PC is then set to nnn.
void call_nnn(uint16_t nnn) {
  printf("CALL 0x%X\n", nnn);

  // Increment the stack pointer
  SP += 1;

  // these might need a swap over!
  stack[SP] = registers.PC;

  // Set PC to nnn
  registers.PC = nnn;
}

// 6xkk - LD Vx, byte
// LD Vx, byte
void ld_vx_yy(uint8_t vx, uint8_t yy) {
  printf("LD V%X, 0x%X\n", vx, opcode & 0x00ff);
  switch(vx) {
    case 0x0:
      registers.V0 = opcode & 0x00ff;
      break;
    case 0x1:
      registers.V1 = opcode & 0x00ff;
      break;
    case 0x2:
      registers.V2 = opcode & 0x00ff;
      break;
    case 0x3:
      registers.V3 = opcode & 0x00ff;
      break;
    case 0x4:
      registers.V4 = opcode & 0x00ff;
      break;
    case 0x5:
      registers.V5 = opcode & 0x00ff;
      break;
    case 0x6:
      registers.V6 = opcode & 0x00ff;
      break;
    case 0x7:
      registers.V7 = opcode & 0x00ff;
      break;
    case 0x8:
      registers.V8 = opcode & 0x00ff;
      break;
    case 0x9:
      registers.V9 = opcode & 0x00ff;
      break;
    case 0xA:
      registers.VA = opcode & 0x00ff;
      break;
    case 0xB:
      registers.VB = opcode & 0x00ff;
      break;
    case 0xC:
      registers.VC = opcode & 0x00ff;
      break;
    case 0xD:
      registers.VD = opcode & 0x00ff;
      break;
    case 0xE:
      registers.VE = opcode & 0x00ff;
      break;
    case 0xF:
      registers.VF = opcode & 0x00ff;
      break;
  }
  // Increment the PC by 2
  registers.PC += 2;
}

// LD I, addr
void ld_i_nnn(uint8_t nnn) {
  printf("LD I, 0x%X\n", nnn);
  registers.I = nnn;
  registers.PC += 2;
}

// JP V0, addr
// Jump to location nnn + V0.
void jp_v0_nnn(uint16_t nnn) {
  // The program counter is set to nnn plus the value of V0.
  registers.PC = registers.V0 + nnn;
}

// Cxkk - RND Vx, byte
// Set Vx = random byte AND kk.
// The interpreter generates a random number from 0 to 255, 
// which is then ANDed with the value kk. The results are stored in Vx.
void rnd_vx_yy(uint8_t vx, uint8_t yy) {

}

// Dxyn - DRW Vx, Vy, nibble
// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
// The interpreter reads n bytes from memory, starting at the address stored in I.
// These bytes are then displayed as sprites on screen at coordinates (Vx, Vy).
// Sprites are XORed onto the existing screen. If this causes any pixels to be 
// erased, VF is set to 1, otherwise it is set to 0. If the sprite is positioned
// so part of it is outside the coordinates of the display, it wraps around to
// the opposite side of the screen. See instruction 8xy3 for more information
// on XOR, and section 2.4, Display, for more information on the Chip-8 screen
// and sprites.
void drw_vx_vy(uint8_t x, uint8_t y, uint8_t n) {
  printf("DRW V%X, V%X, %X\n", x, y, n);

  uint8_t x_val = get_vreg(x);
  uint8_t y_val = get_vreg(y);
  uint8_t pixel;

  // Zero out the carry/collision flag
  registers.VF = 0;

  // Lines
  for (int yline = 0; yline < n; yline++) {
    pixel = memory[registers.I + yline];
    // Each sprite is only 8 bits line
    for (int xline = 0; xline < 8; xline++) {
      // Run through each pixel at a time to check it's on/off
      if ((pixel & 0b10000000) != 0) {
        // If the current pixel is set... we need to turn on the collision flag
        if (gfx[x + xline + ((y + yline) * 64)] == 1) {
          registers.VF = 1;
        }
        // Update the graphics buffer
        gfx[x + xline + ((y + yline) * 64)] ^= 1;
      }
    }
  }

  // toggle the draw flag in the loop
  drawFlag = 1;
  registers.PC += 2;
}

// Fx33 - LD B, Vx
// Store BCD representation of Vx in memory locations I, I+1, and I+2.
// The interpreter takes the decimal value of Vx, and places the hundreds
// digit in memory at location in I, the tens digit at location I+1, and the
// ones digit at location I+2.
void ld_b_vx(uint8_t x) {
  printf("LD B, V%X\n", x);

  // Store BCD representation of Vx in memory locations I, I+1, and I+2.
  uint8_t current_val = get_vreg(x);

  // Store the representation in memory
  memory[registers.I] = current_val / 100;
  memory[registers.I + 1] = current_val / 10 % 10;
  memory[registers.I + 2] = current_val % 10;

  registers.PC += 2;
}

// Fx65 - LD Vx, [I]
// The interpreter reads values from memory starting at location I
// into registers V0 through Vx.
void ld_vx_i(uint8_t x) {

  for (int i = 0; i < (int)x; i++) {
    printf("%d, %x\n", x, memory[registers.I + i]);
  }

  registers.PC += 2;
}

// Initializes all values where needed for the architecture.
void initialize() {

  // Load fontset
  for (int i = 0; i < 80; i++) {
    memory[i] = chip8_fontset[i];
  }

  // Reset timers
  delay_timer = 0;
  sound_timer = 0;

  printf("Loading ROM...\n");

  // Load the ROM file
  int bufferSize = 2048;
  uint8_t buffer[bufferSize];
  FILE *fp = fopen("pong.rom", "rb");

  size_t read_bytes = fread(buffer, 1, bufferSize, fp);
  printf("Read %zu\n", read_bytes);

  // Close the ROM file
  fclose(fp);

  // Load ROM into memory
  printf("Loading ROM into memory...\n");
  for (int i = 0; i < bufferSize; i++) {
    memory[512 + i] = buffer[i];
  }
}

// Emulates the actual CPU clock cycle.
void emulate_cycle() {

  // Fetch opcode
  opcode = memory[registers.PC] << 8 | memory[registers.PC + 1];

  // Decode opcode
  switch(opcode & 0xF000) {

    case 0x2000: // CALL addr
      call_nnn(opcode & 0x0fff);
      break;

    case 0x6000: // LD Vx, byte
      ld_vx_yy((opcode & 0x0f00) >> 8, opcode & 0x00ff);
      break;

    case 0xA000: // LD I, addr
      ld_i_nnn(opcode & 0x0fff);
      break;

    // case 0xB000: // JP V0, addr
    //   break;

    // case 0xC000: // RND Vx, byte
    //   break;

    case 0xD000: // DRW Vx, Vy, nibble
      // Example: 0xDAB6
      drw_vx_vy((opcode & 0xf00) >> 8, (opcode & 0x00f0) >> 4, opcode & 0x000f);
      break;

    case 0xF000:
      printf("opcode: 0x%X\n", opcode);
      switch(opcode & 0x00ff) {
        case 0x33:
          ld_b_vx((opcode & 0x0f00) >> 8);
          break;

        case 0x65:
          ld_vx_i((opcode & 0x0f00) >> 8);
          break;

        default:
          break;
      }
      break;

    default:
      printf("Unknown opcode: 0x%X\n", opcode);
  }

  // Update timers
  if (delay_timer > 0) {
    --delay_timer;
  }

  if (sound_timer > 0) {
    if (sound_timer == 1) {
      printf("BEEP!\n");
    }
    --sound_timer;
  }

}
