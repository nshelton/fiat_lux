

uint8_t ca_grid[NUM_LEDS];
uint8_t ca_grid_swap[NUM_LEDS];

void init_ca() {
  for (int i = 0; i < NUM_LEDS; i++) { 
    ca_grid[i] = random(0,2); 
    ca_grid_swap[i] = random(0,2); 
  }

}

inline void set_ca(uint32_t x, uint32_t y, uint8_t val) {
  ca_grid[(x %WIDTH) + WIDTH * (y % HEIGHT)] = val;
}

inline uint8_t get_ca(uint32_t x, uint32_t y) {
  return ca_grid[(x % WIDTH) + WIDTH * (y % HEIGHT)];
}

inline void set_ca_swap(uint32_t x, uint32_t y, uint8_t val) {
  ca_grid_swap[(x %WIDTH) + WIDTH * (y % HEIGHT)] = val;
}

inline uint8_t get_ca_swap(uint32_t x, uint32_t y) {
  return ca_grid_swap[(x % WIDTH) + WIDTH * (y % HEIGHT)];
}

uint8_t m_binrule[] = {0,0, 0, 1,1,1,1,0};

uint8_t next_state_ca(uint8_t a, uint8_t b, uint8_t c ) {
  return m_binrule[a*4 + b*2 + c];
}

void next_ca() {

  for (uint8_t y = 1; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      set_ca_swap(x, y, get_ca(x, y-1));
    }
  }

  for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t next_state = next_state_ca(get_ca_swap(x-1, 1), get_ca_swap(x, 1), get_ca_swap(x+1, 1));
      set_ca(x, 0, next_state);
  }

  for (int y = 1; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      set_ca(x, y, get_ca_swap(x, y));
    }
  }

}

