Arduino Library for the AccessTable classes.

By David Beaudette

Integrates in the nano-rfid-commutator and nano-rfid-doorlock projects.

Uses the builtin EEPROM from the Arduino by default.
If the EEPROM_SPI symbol is #define'd, will use an external EEPROM
with chip select on pin 1 (D1/TX).

When using the builtin EEPROM, the total number of users is written in 
the 2 last bytes.

When using the external EEPROM, the total number of users is the sum
of the 2 last bytes from all pages. There are 32 users per page:
- 32 * 4 first bytes are the tags
- 32 * 1 authorisation bits start at address 128 of each page
- 2 bytes at address 253 provide the number of users saved in the page