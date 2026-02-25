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
  memset(input_buffer, 0, sizeof(input_buffer));

  for(int i = 0; i < 20; i++){
    memset(screen_buffer[i], ' ', sizeof(screen_buffer[i]));
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
        if(cursor_pos_x > 0){
          cursor_pos_x = (cursor_pos_x - 1) % 64;
          cursor_pos_y = (cursor_pos_x / 64);
          int pos = (cursor_pos_x-22) + cursor_pos_y%64;
          shift_text_left(pos, input_buffer);
        }
      } else if (packet.keycode[0] == 0x28) {
        // ENTER
        input_buffer[cursor_pos_x] = '\n';
        write(sockfd, input_buffer, strlen(input_buffer));
        memset(input_buffer, ' ', BUFFER_SIZE);
        cursor_pos_x = 22;
        cursor_pos_y = 0;
      } else {
        char c = (packet.modifiers & USB_LSHIFT) || (packet.modifiers & USB_RSHIFT) ? usb_to_ascii_shift[packet.keycode[1]]: usb_to_ascii[packet.keycode[1]];
        fbputchar(c, cursor_pos_y, cursor_pos_x);
        input_buffer[cursor_pos_x] = c;
        cursor_pos_x = (cursor_pos_x + 1) % 64;
        cursor_pos_y = (cursor_pos_x / 64);

        c = (packet.modifiers & USB_LSHIFT) || (packet.modifiers & USB_RSHIFT) ? usb_to_ascii_shift[packet.keycode[2]]: usb_to_ascii[packet.keycode[2]];
        fbputchar(c, cursor_pos_y, cursor_pos_x);
        input_buffer[cursor_pos_x] = c;
        cursor_pos_x = (cursor_pos_x + 1) % 64;
        cursor_pos_y = (cursor_pos_x / 64);
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
