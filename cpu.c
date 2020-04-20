/*

Handles the CPU architecture of the CHIP-8

Includes all instructions

*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "cpu.h"

// Flag for whether to update the graphics output or not
int drawFlag;

// Initialize the registers to 0
uint8_t registers[16];

// 16-bit index register
uint16_t I;

// 16-bit program counter
uint16_t PC = 0x200; // Program counter starts at 0x200

// Stack setup
uint16_t stack[16];
// Stack pointer
uint8_t SP;

// Memory
// CHIP-8 has 4k of main memory
uint8_t memory[4096];

// Current opcode
uint16_t opcode;

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

// Get a registers value
uint8_t get_vreg(uint8_t vreg) {
  return registers[vreg];
}

// Log out a message
// TODO: Expand with other log options
void logger(const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  vprintf(pattern, args);
  va_end(args);
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

// 0nnn - SYS addr
// Jump to a machine code routine at nnn.
// This instruction is only used on the old computers on which Chip-8 was
// originally implemented. It is ignored by modern interpreters.
void sys(uint16_t nnn) {
  logger("SYS %X\n", nnn);
  PC = nnn;
}

// 00E0 - CLS
// Clear the display.
void cls() {
  logger("CLS\n");
  memset(gfx, 0, 64 * 32);
  PC += 2;
}

// 00EE - RET
// Return from a subroutine.
// The interpreter sets the program counter to the address at the top of the
// stack, then subtracts 1 from the stack pointer.
void ret() {
  logger("RET\n");
  PC = stack[SP];
  SP--;
}

// 1nnn - JP addr
// Jump to location nnn.
// The interpreter sets the program counter to nnn.
void jp(uint16_t addr) {
  logger("JP 0x%X\n", addr);
  PC = addr;
}

// 2nnn - CALL addr
// Call subroutine at nnn.
// The interpreter increments the stack pointer, then puts the current PC on
// the top of the stack. The PC is then set to nnn.
void call_nnn(uint16_t nnn) {
  logger("CALL 0x%X\n", nnn);

  // Increment the stack pointer
  SP += 1;

  stack[SP] = PC + 2;

  // Set PC to nnn
  PC = nnn;
}

// 3xkk - SE Vx, byte
// Skip next instruction if Vx = kk.
// The interpreter compares register Vx to kk, and if they are equal,
// increments the program counter by 2.
void se_vx_yy(uint8_t x, uint8_t yy) {
  logger("SE V%X, 0x%X\n", x, yy);

  if (registers[x] == yy) {
    PC += 4;
  } else {
    PC += 2;
  }
}

// 4xkk - SNE Vx, byte
// Skip next instruction if Vx != kk.
// The interpreter compares register Vx to kk, and if they are not equal,
// increments the program counter by 2.
void sne_vx_yy(uint8_t x, uint8_t yy) {
  logger("SNE V%X, %X\n", x, yy);

  if (registers[x] != yy) {
    PC += 4;
  } else {
    PC += 2;
  }
}

// 6xkk - LD Vx, byte
// LD Vx, byte
void ld_vx_yy(uint8_t vx, uint8_t yy) {
  logger("LD V%X, 0x%X\n", vx, yy);
  registers[vx] = yy;

  // Increment the PC by 2
  PC += 2;
}

// 7xkk - ADD Vx, byte
// Set Vx = Vx + kk.
// Adds the value kk to the value of register Vx, then stores the result in Vx.
void add_vx_yy(uint8_t x, uint8_t yy) {
  logger("ADD V%X, 0x%x\n", x, yy);
  registers[x] = registers[x] + yy;

  PC += 2;
}

// 8xy0 - LD Vx, Vy
// Set Vx = Vy.
// Stores the value of register Vy in register Vx.
void ld_vx_vy(uint8_t x, uint8_t y) {
  logger("LD V%X, V%X\n", x, y);

  registers[x] = registers[y];

  PC += 2;
}

// 8xy2 - AND Vx, Vy
// Set Vx = Vx AND Vy.
// Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx.
// A bitwise AND compares the corrseponding bits from two values, and if both bits
// are 1, then the same bit in the result is also 1. Otherwise, it is 0.
void and_vx_vy(uint8_t x, uint8_t y) {
  logger("AND V%X, V%X\n", x, y);

  registers[x] = registers[x] & registers[y];

  PC += 2;
}

// 8xy4 - ADD Vx, Vy
// Set Vx = Vx + Vy, set VF = carry.
// The values of Vx and Vy are added together. If the result is greater than 8
// bits (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the
// result are kept, and stored in Vx.
void add_vx_vy(uint8_t x, uint8_t y) {
  logger("ADD V%X, V%X\n", x, y);

  if (registers[x] > (255 - registers[y])) {
    registers[VF] = 1;
  } else {
    registers[VF] = 0;
  }

  // Store result in Vx
  registers[x] = registers[x] + registers[y];

  PC += 2;
}

// 8xy5 - SUB Vx, Vy
// Set Vx = Vx - Vy, set VF = NOT borrow.
// If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx,
// and the results stored in Vx.
void sub_vx_vy(uint8_t x, uint8_t y) {
  logger("SUB V%X, V%X\n", x, y);

  if (registers[x] > registers[y]) {
    registers[VF] = 1;
  } else {
    registers[VF] = 0;
  }

  registers[x] = registers[x] - registers[y];

  PC += 2;

}

// Annn - LD I, addr
// Set I = nnn.
// The value of register I is set to nnn.
void ld_i_nnn(uint16_t nnn) {
  logger("LD I, 0x%X\n", nnn);
  I = nnn;
  PC += 2;
}

// Bnnn - JP V0, addr
// Jump to location nnn + V0.
void jp_v0_nnn(uint16_t nnn) {
  logger("JP V0, 0x%X\n", nnn);
  // The program counter is set to nnn plus the value of V0.
  PC = registers[V0] + nnn;
}

// Cxkk - RND Vx, byte
// Set Vx = random byte AND kk.
// The interpreter generates a random number from 0 to 255,
// which is then ANDed with the value kk. The results are stored in Vx.
void rnd_vx_yy(uint8_t x, uint8_t yy) {
  logger("RND V%X, %X\n", x, yy);

  registers[x] = (rand() % 255) & yy;

  PC += 2;
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
  logger("DRW V%X, V%X, %X\n", x, y, n);

  uint8_t x_val = get_vreg(x);
  uint8_t y_val = get_vreg(y);
  uint8_t pixel = 0;

  // Zero out the carry/collision flag
  registers[VF] = 0;

  // Lines
  for (int yline = 0; yline < n; yline++) {
    pixel = memory[I + yline];
    // Each sprite is only 8 bits long
    for (int xline = 0; xline < 8; xline++) {
      // Run through each pixel at a time to check it's on/off
      if ((pixel & (0b10000000 >> xline)) != 0) {
        // If the current pixel is set... we need to turn on the collision flag
        if (gfx[x_val + xline + ((y_val + yline) * 64) % (64 * 32)] == 1) {
          registers[VF] = 1;
        }
        // Update the graphics buffer
        gfx[x_val + xline + ((y_val + yline) * 64) % (64 * 32)] ^= 1;
      }
    }
  }

  // toggle the draw flag in the loop
  drawFlag = 1;
  PC += 2;
}

// ExA1 - SKNP Vx
// Skip next instruction if key with the value of Vx is not pressed.
// Checks the keyboard, and if the key corresponding to the value of
// Vx is currently in the up position, PC is increased by 2.
void sknp_vx(uint8_t x) {
  logger("SKNP V%X\n", x);

  if ((key[x] & 1) != 1) {
    PC += 4;
  } else {
    PC += 2;
  }
}

// Fx07 - LD Vx, DT
// Set Vx = delay timer value.
// The value of DT is placed into Vx.
void ld_vx_dt(uint8_t x) {
  logger("LD V%X, DT\n", x);
  registers[x] = delay_timer;
  PC += 2;
}

// Fx15 - LD DT, Vx
// Set delay timer = Vx.
// DT is set equal to the value of Vx.
void ld_dt_vx(uint8_t x) {
  logger("LD DT, V%X\n", x);
  delay_timer = registers[x];
  PC += 2;
}

// Fx18 - LD ST, Vx
// Set sound timer = Vx.
// ST is set equal to the value of Vx.
void ld_st_vx(uint8_t x) {
  logger("LD ST, V%X\n", x);
  sound_timer = registers[x];
  PC += 2;
}

// Fx1E - ADD I, Vx
// Set I = I + Vx.
// The values of I and Vx are added, and the results are stored in I.
void add_i_vx(uint8_t x) {
  logger("ADD I, V%X\n", x);
  I += registers[x];
  PC += 2;
}

// Fx29 - LD F, Vx
// Set I = location of sprite for digit Vx.
// The value of I is set to the location for the hexadecimal sprite corresponding
// to the value of Vx. See section 2.4, Display, for more information on the
// Chip-8 hexadecimal font.
void ld_f_vx(uint8_t x) {
  logger("LD F, V%X\n", x);
  // logger("Loading sprite %d\n", registers[x] * 5);
  I = registers[x] * 5;
  PC += 2;
}

// Fx33 - LD B, Vx
// Store BCD representation of Vx in memory locations I, I+1, and I+2.
// The interpreter takes the decimal value of Vx, and places the hundreds
// digit in memory at location in I, the tens digit at location I+1, and the
// ones digit at location I+2.
void ld_b_vx(uint8_t x) {
  logger("LD B, V%X\n", x);

  // Store BCD representation of Vx in memory locations I, I+1, and I+2.
  uint8_t current_val = get_vreg(x);

  // Store the representation in memory
  memory[I] = current_val / 100;
  memory[I + 1] = current_val / 10 % 10;
  memory[I + 2] = current_val % 10;

  PC += 2;
}

// Fx65 - LD Vx, [I]
// The interpreter reads values from memory starting at location I
// into registers V0 through Vx.
void ld_vx_i(uint8_t x) {
  logger("LD V%X, [I]\n", x);

  for (int i = 0; i <= x; i++) {
    registers[i] = memory[I + i];
  }

  PC += 2;
}

// Initializes all values where needed for the architecture.
void initialize(uint8_t *game, size_t game_size) {

  // Load fontset
  for (int i = 0; i < 80; i++) {
    memory[i] = chip8_fontset[i];
  }

  // Reset timers
  delay_timer = 0;
  sound_timer = 0;

  logger("Loading ROM...\n");

  // Load the ROM file
  int bufferSize = 2048;
  uint8_t buffer[bufferSize];
  memcpy(buffer, game, game_size);
  logger("Read %zu\n", game_size);

  // Load ROM into memory
  logger("Loading ROM into memory...\n");
  for (int i = 0; i < bufferSize; i++) {
    memory[512 + i] = buffer[i];
  }
}

void update_timers() {
  // Update timers
  if (delay_timer > 0) {
    --delay_timer;
  }

  if (sound_timer > 0) {
    if (sound_timer == 1) {
      printf("****** BEEP! ******\n");
    }
    --sound_timer;
  }
}

// Emulates the actual CPU clock cycle.
void emulate_cycle() {

  // Fetch opcode
  opcode = memory[PC] << 8 | memory[PC + 1];

  logger("0x%X - OC: 0x%X - ", PC, opcode);

  // Decode opcode
  switch(opcode & 0xF000) {

    case 0x00:
      switch(opcode & 0x00ff) {
        case 0x00: // SYS addr
          sys(opcode & 0x0fff);
          break;

        case 0xE0: // CLS
          cls();
          break;

        case 0xEE: // RET
          ret();
          break;

        default:
          logger("Unknown opcode: in 0x0: 0x%X\n", opcode);
          break;
      }
      break;

    case 0x1000: // JP addr
      jp(opcode & 0x0fff);
      break;

    case 0x2000: // CALL addr
      call_nnn(opcode & 0x0fff);
      break;

    case 0x3000: // SE Vx, byte
      se_vx_yy((opcode & 0x0f00) >> 8, opcode & 0x00ff);
      break;

    case 0x4000: // SNE Vx, byte
      sne_vx_yy((opcode & 0x0f00) >> 8, opcode & 0x00ff);
      break;

    case 0x6000: // LD Vx, byte
      ld_vx_yy((opcode & 0x0f00) >> 8, opcode & 0x00ff);
      break;

    case 0x7000: // ADD Vx, byte
      add_vx_yy((opcode & 0x0f00) >> 8, opcode & 0x00ff);
      break;

    case 0x8000:
      switch(opcode & 0xf) {
        case 0x0: // LD Vx, Vy
          ld_vx_vy((opcode & 0x0f00) >> 8, (opcode & 0x00f0) >> 4);
          break;

        case 0x2: // AND Vx, Vy
          and_vx_vy((opcode & 0x0f00) >> 8, (opcode & 0x00f0) >> 4);
          break;

        case 0x4: // ADD Vx, Vy
          add_vx_vy((opcode & 0x0f00) >> 8, (opcode & 0x00f0) >> 4);
          break;

        case 0x5: // SUB Vx, Vy
          sub_vx_vy((opcode & 0x0f00) >> 8, (opcode & 0x00f0) >> 4);
          break;

        default:
          logger("Unknown opcode: in 0x8: 0x%X\n", opcode);
          break;

      }
      break;

    case 0xA000: // LD I, addr
      ld_i_nnn(opcode & 0x0fff);
      break;

    // case 0xB000: // JP V0, addr
    //   break;

    case 0xC000: // RND Vx, byte
      rnd_vx_yy((opcode & 0x0f00) >> 8, opcode & 0x00ff);
      break;

    case 0xD000: // DRW Vx, Vy, nibble
      // Example: 0xDAB6
      drw_vx_vy((opcode & 0xf00) >> 8, (opcode & 0x00f0) >> 4, opcode & 0x000f);
      break;

    case 0xE000: // SKNP Vx
      sknp_vx((opcode & 0xf00) >> 8);
      break;

    case 0xF000:
      switch(opcode & 0x00ff) {
        case 0x07:
          ld_vx_dt((opcode & 0x0f00) >> 8);
          break;

        case 0x15:
          ld_dt_vx((opcode & 0x0f00) >> 8);
          break;

        case 0x18: // LD ST, Vx
          ld_st_vx((opcode & 0x0f00) >> 8);
          break;

        case 0x1E: // ADD I, Vx
          add_i_vx((opcode & 0x0f00) >> 8);
          break;

        case 0x29:
          ld_f_vx((opcode & 0x0f00) >> 8);
          break;

        case 0x33:
          ld_b_vx((opcode & 0x0f00) >> 8);
          break;

        case 0x65:
          ld_vx_i((opcode & 0x0f00) >> 8);
          break;

        default:
          logger("Unknown opcode: 0x%X\n", opcode);
          break;
      }
      break;

    default:
      logger("Unknown opcode: 0x%X\n", opcode);
      break;
  }

}
