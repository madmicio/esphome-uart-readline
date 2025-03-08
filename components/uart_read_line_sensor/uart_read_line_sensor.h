#include "esphome.h"

class UartReadLineSensor : public Component, public UARTDevice, public TextSensor {
 public:
  UartReadLineSensor(UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override {
    ESP_LOGD("uart_readline", "UartReadLineSensor setup complete.");
  }

  int readline(int readch, char *buffer, int len) {
    static int pos = 0;  // Posizione corrente nel buffer
    int rpos;

    if (readch > 0) {
      switch (readch) {
        case '\n':  // Gestisce sia \n che \r come terminatori
        case '\r':
        case 0x03:  // ETX (End of Text)
          rpos = pos;  // Salva la posizione attuale
          pos = 0;  // Resetta il buffer
          return rpos;  // Ritorna la lunghezza della linea
        default:
          if (pos < len - 1) {
            buffer[pos++] = readch;  // Aggiunge il carattere al buffer
            buffer[pos] = '\0';  // Termina la stringa
          } else {
            ESP_LOGW("uart_readline", "Buffer overflow, resetting.");
            pos = 0;  // Resetta il buffer in caso di overflow
          }
      }
    }
    return -1;  // Nessuna linea completa
  }

  void loop() override {
    const int max_line_length = 80;  // Dimensione massima del buffer
    static char buffer[max_line_length];
    while (available()) {
      int read_character = read();  // Legge un carattere dalla UART
      if (readline(read_character, buffer, max_line_length) > 0) {
        ESP_LOGD("uart_readline", "Complete line received: %s", buffer);

        // Applica il filtro per rimuovere STX (0x02) e ETX (0x03)
        if (buffer[0] == 0x02) {  // Verifica STX
          ESP_LOGD("uart_readline", "Removing STX (0x02)");
          memmove(buffer, buffer + 1, strlen(buffer));  // Rimuove STX
        }
        if (buffer[strlen(buffer) - 1] == 0x03) {  // Verifica ETX
          ESP_LOGD("uart_readline", "Removing ETX (0x03)");
          buffer[strlen(buffer) - 1] = '\0';  // Rimuove ETX
        }

        publish_state(buffer);  // Pubblica il messaggio filtrato
        ESP_LOGD("uart_readline", "Publishing filtered state: %s", buffer);
      }
    }
  }
  
};
