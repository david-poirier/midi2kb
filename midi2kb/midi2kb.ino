
#include "HID-Project.h"
#include <usbh_midi.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>


USB Usb;
USBH_MIDI  Midi(&Usb);

void MIDI_poll();

uint16_t pid, vid;

bool kb_signal_1, kb_signal_2, kb_activated;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  
  vid = pid = 0;
  Serial.begin(115200);

  if (Usb.Init() == -1) {
    while (1); //halt
  }//if (Usb.Init() == -1...
  delay( 200 );
}

void loop()
{
  Usb.Task();
  if ( Midi ) {
    MIDI_poll();
  }
}

void log(char *format, ...) {
  va_list args;
  va_start(args, format);
  char print_msg[1000];
  vsnprintf(print_msg, sizeof(print_msg), format, args);
  Serial.print(print_msg);
  va_end(args);
}

void process_note(bool on, uint8_t midi_key) {
  if (midi_key == 48) {
    kb_signal_1 = on;
  }
  else if (midi_key == 72) {
    kb_signal_2 = on;
  }

  if (kb_signal_1 && kb_signal_2) {
    kb_activated = !kb_activated;
    if (kb_activated) {
      Serial.print("Beginning keyboard emulation\n");
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.end();
      BootKeyboard.begin();
    }
    else {
      Serial.print("Ending keyboard emulation\n");
      digitalWrite(LED_BUILTIN, LOW);
      BootKeyboard.releaseAll();
      BootKeyboard.end();
      Serial.begin(115200);
    }
  }

  if (kb_activated) {
    if (on) {
      BootKeyboard.press((char)midi_key);
    }
    else {
      BootKeyboard.release((char)midi_key);
    }
  }
}


// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
  uint8_t midi_msg[3];
  uint8_t status_1, status_2, data_1, data_2;
  uint8_t size;

  do {
    if ( (size = Midi.RecvData(midi_msg)) > 0 ) {
      status_1 = midi_msg[0] >> 4;
      status_2 = midi_msg[0] & 0x0F;
      data_1 = midi_msg[1];
      data_2 = midi_msg[2];
      
      log("Status: %X (%X, %X)\n", midi_msg[0], status_1, status_2);
      switch (status_1) {
        case 0x8:
          log("Note off - note: %X (%d), velocity: %X (%d)\n", data_1, data_1, data_2, data_2);
          process_note(false, data_1);
          break;
          
        case 0x9:
          log("Note on - note: %X (%d), velocity: %X (%d)\n", data_1, data_1, data_2, data_2);
          process_note(true, data_1);
          break;

        case 0xB:
          log("Control change: %X (%d), value: %X (%d)\n", data_1, data_1, data_2, data_2);
          switch (data_1) {
            case 1:
              log("Mod wheel: %X (%d)\n", data_2, data_2);
              break;
            case 64:
              switch (data_2) {
                case 0:
                  log("Hold pedal off\n");
                  break;
                default:
                  log("Hold pedal on\n");
                  break;
              }
              break;
          }
          break;

        case 0xE:
          uint16_t bend = ((uint16_t)data_1 << 7) | data_2;
          log("Pitch bend: %X (%d)\n", bend, bend);
          break;
      }
    }
  } while (size > 0);
}
