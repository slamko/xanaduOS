#include "drivers/mouse.h"
#include "drivers/fb.h"
#include "drivers/int.h"
#include "drivers/keyboard.h"
#include "lib/kernel.h"
#include <stdint.h>

enum {
    PS2_COMMAND_PORT = 0x64,
    PS2_DATA_PORT = 0x60
};

enum {
    PS2_ENABLE_AUX = 0xA8,
    PS2_ACKNOWLEDGE = 0xFA,
    PS2_GET_COMPAQ_STAT = 0x20,
    PS2_SET_COMPAQ_STAT = 0x60,
    PS2_SET_DEFAULTS = 0xF6,
    PS2_ENABLE_STREAMING = 0xF4,
};

void ps2_handler(struct isr_handler_args args) {
    fb_print_black("mouse");
    uint8_t stat = inb(KBD_STATUS_PORT);


    if (stat & (1 << 5)) {

    }
}

uint8_t ps2_wait_read(void) {
    while(!(inb(0x64) & 1)) asm volatile ("pause");
    fb_newline();
    fb_print_num(inb(PS2_COMMAND_PORT));
    fb_newline();

    return inb(PS2_DATA_PORT);
}

void mouse_wait(uint8_t a_type) //unsigned char
{
  int _time_out=100000; //unsigned int
  if(a_type==0)
  {
    while(_time_out--) //Data
    {
      if((inb(0x64) & 1)==1)
      {
        return;
      }
    }
    return;
  }
  else
  {
    while(_time_out--) //Signal
    {
      if((inb(0x64) & 2)==0)
      {
        return;
      }
    }
    return;
  }
}

/*
int ps2_init(void) {
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_ENABLE_AUX);

    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_GET_COMPAQ_STAT);
    mouse_wait(0);
    uint8_t compaq_stat = inb(PS2_DATA_PORT);
    compaq_stat |= (1 << 1);
    compaq_stat &= ~(1 << 5);

    mouse_wait(1);
    outb(PS2_COMMAND_PORT, PS2_SET_COMPAQ_STAT);
    mouse_wait(1);
    outb(PS2_DATA_PORT, compaq_stat);
    
    ps2_wait_read();

    mouse_wait(1);
    outb(PS2_COMMAND_PORT, 0xD4);
    mouse_wait(1);
    outb(PS2_DATA_PORT, PS2_SET_DEFAULTS);
    mouse_wait(0);
    inb(PS2_DATA_PORT);

    
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, 0xD4);
    mouse_wait(1);
    outb(PS2_DATA_PORT, PS2_ENABLE_STREAMING);
    mouse_wait(0);
    inb(PS2_DATA_PORT);

    add_irq_handler(12, &ps2_handler);
    
    return 0;
}
*/
void mouse_write(uint8_t a_write) //unsigned char
{
  //Wait to be able to send a command
  mouse_wait(1);
  //Tell the mouse we are sending a command
  outb(0x64, 0xD4);
  //Wait for the final part
  mouse_wait(1);
  //Finally write
  outb(0x60, a_write);
}

uint8_t mouse_read()
{
  //Get's response from mouse
  mouse_wait(0);
  return inb(0x60);
}

int ps2_init()
{
  uint8_t _status;  //unsigned char

  //Enable the auxiliary mouse device
  mouse_wait(1);
  outb(0x64, 0xA8);
 
  //Enable the interrupts
  mouse_wait(1);
  outb(0x64, 0x20);
  mouse_wait(0);
  _status=(inb(0x60) | 2);
  mouse_wait(1);
  outb(0x64, 0x60);
  mouse_wait(1);
  outb(0x60, _status);
 
  //Tell the mouse to use default settings
  mouse_write(0xF6);
  mouse_read();  //Acknowledge
 
  //Enable the mouse
  mouse_write(0xF4);
  mouse_read();  //Acknowledge

  add_irq_handler(12, ps2_handler);
  return 0;
}
