/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

#define USB_KEY_MAX 128

const char usb_to_ascii[USB_KEY_MAX] = {
  [0x04] = 'a',
  [0x05] = 'b',
  [0x06] = 'c',
  [0x07] = 'd',
  [0x08] = 'e',
  [0x09] = 'f',
  [0x0A] = 'g',
  [0x0B] = 'h',
  [0x0C] = 'i',
  [0x0D] = 'j',
  [0x0E] = 'k',
  [0x0F] = 'l',
  [0x10] = 'm',
  [0x11] = 'n',
  [0x12] = 'o',
  [0x13] = 'p',
  [0x14] = 'q',
  [0x15] = 'r',
  [0x16] = 's',
  [0x17] = 't',
  [0x18] = 'u',
  [0x19] = 'v',
  [0x1A] = 'w',
  [0x1B] = 'x',
  [0x1C] = 'y',
  [0x1D] = 'z',

  [0x1E] = '1',
  [0x1F] = '2',
  [0x20] = '3',
  [0x21] = '4',
  [0x22] = '5',
  [0x23] = '6',
  [0x24] = '7',
  [0x25] = '8',
  [0x26] = '9',
  [0x27] = '0',

  [0x2C] = ' ',
  [0x2D] = '-',    
  [0x2E] = '=',
  [0x2F] = '[',
  [0x30] = ']',
  [0x31] = '\\',
  [0x33] = ';',
  [0x34] = '\'',
  [0x35] = '`',
  [0x36] = ',',
  [0x37] = '.',
  [0x38] = '/'
};

const char usb_to_ascii_shift[USB_KEY_MAX] = {
  [0x04] = 'A',
  [0x05] = 'B',
  [0x06] = 'C',
  [0x07] = 'D',
  [0x08] = 'E',
  [0x09] = 'F',
  [0x0A] = 'G',
  [0x0B] = 'H',
  [0x0C] = 'I',
  [0x0D] = 'J',
  [0x0E] = 'K',
  [0x0F] = 'L',
  [0x10] = 'M',
  [0x11] = 'N',
  [0x12] = 'O',
  [0x13] = 'P',
  [0x14] = 'Q',
  [0x15] = 'R',
  [0x16] = 'S',
  [0x17] = 'T',
  [0x18] = 'U',
  [0x19] = 'V',
  [0x1A] = 'W',
  [0x1B] = 'X',
  [0x1C] = 'Y',
  [0x1D] = 'Z',

  [0x1E] = '!',
  [0x1F] = '@',
  [0x20] = '#',
  [0x21] = '$',
  [0x22] = '%',
  [0x23] = '^',
  [0x24] = '&',
  [0x25] = '*',
  [0x26] = '(',
  [0x27] = ')',
  [0x2C] = ' ',
  [0x2D] = '_',
  [0x2E] = '+',
  [0x2F] = '{',
  [0x30] = '}',
  [0x31] = '|',
  [0x33] = ':',
  [0x34] = '"',
  [0x35] = '~',
  [0x36] = '<',
  [0x37] = '>',
  [0x38] = '?'
};


/*
 * References:
 *
 * https://web.archive.org/web/20130307100215/http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

pthread_mutex_t screen_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

char screen_buffer[20][64];
char input_buffer[BUFFER_SIZE];

void clear_input_box(){
  memset(input_buffer, 0, sizeof(input_buffer));
  for(int i = 0; i < 64; i++){
    fbputchar(' ', 22, i);
    fbputchar(' ', 23, i);
  }
}

void clear_screen(){
  memset(screen_buffer, 0, sizeof(screen_buffer));
  for(int i = 0; i < 20; i++){
    for(int j = 0; j < 64; j++){
      fbputchar(' ', i, j);
    }
  }
}

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  //memset(input_buffer, 0, sizeof(input_buffer));

  int cursor_pos = 0;

  int prev_char1 = 0;
  int prev_char2 = 0;

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  clear_input_box();
  clear_screen();

  fbputs("Welcome to the CSEE 4840 Chat!", 0, 1);

  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 21, col);
  }

  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);

    fbcursor(cursor_pos, input_buffer);
    if (transferred == sizeof(packet)) {

      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);
      printf("Pressed key: %s\n", keystate);
      fbputs(keystate, 0, 35);

      // ESCAPE
      if (packet.keycode[0] == 0x29) {
        clear_screen();
        clear_screen();
        close(sockfd);
        break;
      }
      else if (packet.keycode[0] == 0x2a){ 
        // BACKSPACE
        if(cursor_pos > 0)
          cursor_pos--;
        shift_text_left(cursor_pos, input_buffer);
      }
      else if(packet.keycode[0] == 0x4c) {
        // DELETE
        if(cursor_pos < strlen(input_buffer)){
          shift_text_left(cursor_pos, input_buffer);
        }
      }
      else if (packet.keycode[0] == 0x28) {
        // ENTER
        if(strlen(input_buffer) == 0)
          continue;
        input_buffer[cursor_pos] = '\n';
        write(sockfd, input_buffer, strlen(input_buffer));
        cursor_pos = 0;
        clear_input_box();
      }
      else if(packet.keycode[0] >= 0x4f && packet.keycode[0] <= 0x52){ 
        //arrow keys
        if(packet.keycode[0] == 0x50){ 
          //left arrow and check 0
          if(cursor_pos > 0){
            fbputchar(cursor_pos>strlen(input_buffer) ? ' ' : input_buffer[cursor_pos], 22 + cursor_pos / 64, cursor_pos % 64);
            cursor_pos--;
          }
        }else if(packet.keycode[0] == 0x4f){ 
          //right arrow and check less than buffer size
          if(cursor_pos < strlen(input_buffer)){
            fbputchar(input_buffer[cursor_pos], 22 + cursor_pos / 64, cursor_pos % 64);
            cursor_pos++;
          }
        }else if(packet.keycode[0] == 0x52){ 
          //up arrow
          if(cursor_pos - 64 >= 0){
            fbputchar(input_buffer[cursor_pos], 22 + cursor_pos / 64, cursor_pos % 64);
            cursor_pos -= 64;
          }
        }else {
          //down arrow
          if(cursor_pos + 64 <= strlen(input_buffer)){
            fbputchar(input_buffer[cursor_pos], 22 + cursor_pos / 64, cursor_pos % 64);
            cursor_pos += 64;
          }
        }
      } 
      else {
        int check_shift = (packet.modifiers & USB_LSHIFT) || (packet.modifiers & USB_RSHIFT);
        char c1 = check_shift ? usb_to_ascii_shift[packet.keycode[0]]: usb_to_ascii[packet.keycode[0]];
        char c2 = check_shift ? usb_to_ascii_shift[packet.keycode[1]]: usb_to_ascii[packet.keycode[1]];

        if(!(packet.keycode[0] == prev_char1 || packet.keycode[0] == prev_char2 || packet.keycode[0] == 0) && cursor_pos < BUFFER_SIZE-1)
        {
          printf("Pressed key: %c\n", c1);
          fbputchar(c1, 22 + cursor_pos / 64, cursor_pos % 64);
          input_buffer[cursor_pos] = c1;
          cursor_pos++;
        }        
        prev_char1 = packet.keycode[0];

        if(!(packet.keycode[1] == prev_char2 || packet.keycode[1] == 0) && cursor_pos < BUFFER_SIZE-1)
        {
          printf("Pressed key: %c\n", c2);
          fbputchar(c2, 22 + cursor_pos / 64, cursor_pos % 64);
          input_buffer[cursor_pos] = c2;
          cursor_pos++;
        }
        prev_char2 = packet.keycode[1];
      }
    }
    usleep(10000);
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  /* Receive data */
  memset(recvBuf, 0, sizeof(recvBuf));
  while ((n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("Received: %s\n", recvBuf);
    //fbputs(recvBuf, 8, 0);

    // Lock the screen buffer mutex before updating the screen buffer
    pthread_mutex_lock(&screen_buffer_mutex);
    screen_shift(screen_buffer, recvBuf);
    // Unlock the screen buffer mutex after updating the screen buffer  
    pthread_mutex_unlock(&screen_buffer_mutex);

  }
  return NULL;
}
