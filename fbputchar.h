#ifndef _FBPUTCHAR_H
#  define _FBPUTCHAR_H

#define FBOPEN_DEV -1          /* Couldn't open the device */
#define FBOPEN_FSCREENINFO -2  /* Couldn't read the fixed info */
#define FBOPEN_VSCREENINFO -3  /* Couldn't read the variable info */
#define FBOPEN_MMAP -4         /* Couldn't mmap the framebuffer memory */
#define FBOPEN_BPP -5          /* Unexpected bits-per-pixel */

typedef enum {LEFT, RIGHT} direction_t;

extern int fbopen(void);
extern void fbputchar(char, int, int);
extern void fbputs(const char *, int, int);
extern void fbcursor(int, int, char *);
extern void screen_shift(char screen_buffer[20][64], char *new_content);
extern void shift_text_left(int pos, char * input_buffer);

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

#endif
