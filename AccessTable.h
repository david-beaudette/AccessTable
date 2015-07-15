/**
 * AccessTable.h
 * 
 * Defines the list of user ID's and associated authorisations.
 **/
 
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

#include "Arduino.h"

#define NOMINAL_TAG_LEN    4 // Nominal length of a user tag
#define AUTH_OFFSET_SHIFT  3 // Number of rightshift bits to obtain 
                             // authorisation address offset from 
                             // tag address offset


#if defined(EEPROM_SPI)// Using external SPI EEPROM Microchip 25LC1024 (1Mbit)

#include <spieeprom.h>

#define PAGE_SIZE        256 // Number of bytes per page  
#define NUM_PAGES        512 // Number of pages in EEPROM  
#define USERS_PER_PAGE    32 // Number of users per memory page
#define MAX_USER_SIZE  16384 // Limit on total number of users
#define TAG_OFFSET_MASK 0x7F // Mask to obtain tag address offset 
                             // from index
#define TAG_OFFSET_SHIFT   2 // Number of leftshift bits to obtain 
                             // tag address from masked index
#define PAGE_OFFSET_SHIFT  5 // Number of rightshift bits to obtain 
                             // page address from index
const unsigned int userStartAddr = 0; // Start of tags on a page
// Start of authorisations on a page
const unsigned int authStartAddr = userStartAddr + 
                                   NOMINAL_TAG_LEN * USERS_PER_PAGE;
// Start of user count on a page
const unsigned int userCountAddr = PAGE_SIZE - 3;

#else // Using builtin Arduino EEPROM

#include <EEPROM.h>

#define NUM_PAGES          1 // Number of pages in EEPROM  
#define MAX_USER_SIZE    247 // Limit on total number of users
#define EEPROM_SIZE     1024
#define NOMINAL_TAG_LEN    4

const unsigned int userStartAddr = 0; // Start of tags in memory
// Start of authorisations in memory
const unsigned int authStartAddr = userStartAddr + 
                                   NOMINAL_TAG_LEN * MAX_USER_SIZE;
// Start of user count in memory
const unsigned int userCountAddr = EEPROM_SIZE - 3;

#endif  // EEPROM_SPI

class AccessTable {
  public:
    AccessTable();
    AccessTable(int pin_num);
    unsigned int getNumUsers();    
    int getUserAuth(byte *tag_id, int num_bytes = NOMINAL_TAG_LEN);
    int setUserAuth(byte *tag_id, byte auth);
    int addUser(byte *tag_id, byte auth);
    int clearTable();
    void print_table();
    
  private:
    int setNumUsers(unsigned int numUsers);
    int setAuth(int tableIndex, byte auth);
    int getAuth(int tableIndex);
    int getUserIndex(byte *tag_id, int num_bytes = NOMINAL_TAG_LEN);
    byte readMemory(long address);
    void writeMemory(long address, byte value);
    void writeMemoryArray(long address, byte *value, int length);

#if defined(EEPROM_SPI) 
    byte _page_data[PAGE_SIZE];
    SPIEEPROM *_spi_eeprom;
#endif  // EEPROM_SPI  
};


#endif // __ACCESSTABLE__
