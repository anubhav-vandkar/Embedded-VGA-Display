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

  [0x28] = '\n',
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

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  char input_buffer[BUFFER_SIZE];
  memset(input_buffer, ' ', sizeof(input_buffer));

  
  for(int i = 0; i < 20; i++){
    for(int j = 0; j < 64; j++){
      screen_buffer[i][j] = ' ';
    }
  }

  int cursor_pos_x = 0;
  int cursor_pos_y = 22;

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  /* Clear screen */
  for(int i=0; i < 20; i++){
    fbputs(screen_buffer[i], i+1, 0);
  }

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

    fbcursor(cursor_pos_y, cursor_pos_x, input_buffer);
    if (transferred == sizeof(packet)) {

      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);
      printf("Pressed key: %s\n", keystate);
      fbputs(keystate, 0, 35);

      //Handle only escape and call function
      // ESCAPE
      if (packet.keycode[0] == 0x29) {
        memset(input_buffer, 0, BUFFER_SIZE);
        memset(screen_buffer, 0, sizeof(screen_buffer));
        for(int i=0; i < 20; i++){
          fbputs(screen_buffer[i], i+1, 0);
        }
        break;
      }else if (packet.keycode[0] == 0x2a) {
        // BACKSPACE
        if(cursor_pos_x != 0 && cursor_pos_y == 22 ){
          cursor_pos_x = (64 + cursor_pos_x - 1) % 64;
          int pos = cursor_pos_x + (cursor_pos_y - 22) * 64;
          shift_text_left(pos, input_buffer);
        } else if (cursor_pos_y == 23) {
          cursor_pos_y = cursor_pos_x ? 22 : 23;
          cursor_pos_x = (64 + cursor_pos_x - 1) % 64;
          int pos = cursor_pos_x + (cursor_pos_y - 22) * 64;
          shift_text_left(pos, input_buffer);
        }
      }else if (packet.keycode[0] == 0x28) {
        // ENTER
        input_buffer[cursor_pos_x] = '\n';
        write(sockfd, input_buffer, strlen(input_buffer));
        memset(input_buffer, ' ', BUFFER_SIZE);
        cursor_pos_y = 22;
        cursor_pos_x = 0;
      }else if(packet.keycode[0] == 0x50 || packet.keycode[0] == 0x4f){ 
        //left and right arrow keys
        if(packet.keycode[0] == 0x50){ //left arrow
          if(cursor_pos_x > 0){
            cursor_pos_y = 22 + (cursor_pos_x - 1) / 64;
            cursor_pos_x = (cursor_pos_x - 1) % 64;
          }
        }else if(packet.keycode[0] == 0x4f){ //right arrow
          if(cursor_pos_x < BUFFER_SIZE){
            cursor_pos_y = 22 + (cursor_pos_x + 1) / 64;
            cursor_pos_x = (cursor_pos_x + 1) % 64;
          }
        }else if(packet.keycode[0] == 0x48 || packet.keycode[0] == 0x50){ 
          if(packet.keycode[0] == 0x48){ //up arrow
            if(cursor_pos_y > 22)
              cursor_pos_y--;
          }else if(packet.keycode[0] == 0x50){ //down arrow
            if(cursor_pos_y < 23)
              cursor_pos_y++;
          }
        }
      } 
      else {
        if(packet.keycode[0] == 0x00 || (cursor_pos_x + (cursor_pos_y - 22) * 64 >= BUFFER_SIZE))
          continue; // ignore if buffer is full
        
        char c = (packet.modifiers & USB_LSHIFT) || (packet.modifiers & USB_RSHIFT) ? usb_to_ascii_shift[packet.keycode[0]]: usb_to_ascii[packet.keycode[0]];
        printf("Pressed key: %c\n", c);
        fbputchar(c, cursor_pos_y, cursor_pos_x);
        input_buffer[cursor_pos_x + (cursor_pos_y - 22) * 64] = c;
        cursor_pos_y = cursor_pos_y + (cursor_pos_x + 1)/64;
        cursor_pos_x = (cursor_pos_x + 1) % 64;

        if(packet.keycode[1] == 0x00) 
          continue; // ignore if not pressed

        c = (packet.modifiers & USB_LSHIFT) || (packet.modifiers & USB_RSHIFT) ? usb_to_ascii_shift[packet.keycode[1]]: usb_to_ascii[packet.keycode[1]];
        printf("Pressed key: %c\n", c);
        fbputchar(c, cursor_pos_y, cursor_pos_x);
        input_buffer[cursor_pos_x + (cursor_pos_y - 22) * 64] = c;
        cursor_pos_y = cursor_pos_y + (cursor_pos_x + 1)/64;
        cursor_pos_x = (cursor_pos_x + 1) % 64;
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
  memset(recvBuf, ' ', sizeof(recvBuf));
  while ((n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("Received: %s\n", recvBuf);
    //fbputs(recvBuf, 8, 0);

    // LOCK SCREEN BUFFER
    pthread_mutex_lock(&screen_buffer_mutex);
    screen_shift(screen_buffer, recvBuf);
    // UNLOCK SCREEN BUFFER
    pthread_mutex_unlock(&screen_buffer_mutex);
  }
  return NULL;
}
