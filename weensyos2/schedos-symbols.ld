/* Define the location of the 'cursorpos' symbol. */

PROVIDE(cursorpos = 0x198000);

/* Define the location of the 'mutex' symbol. */
/* 'cursorpos' occupies 0x198000-0x198003 */
PROVIDE(mutex = 0x198004);
