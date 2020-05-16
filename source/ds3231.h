#include <stdint.h>

void ds_init();
void ds_set_addr(uint8_t _addr);
void ds_write(uint8_t _byte);
uint8_t ds_read(void);
void ds_end_transaction(void);
