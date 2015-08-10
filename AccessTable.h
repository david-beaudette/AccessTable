/**
 * AccessTable.h
 * 
 * Defines the list of user ID's and associated authorisations.
 **/
 
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

#include "Arduino.h"

#define NOMINAL_TAG_LEN    4 // Nominal length of a user tag


#if defined(EEPROM_SPI)// Using external SPI EEPROM Microchip 25LC1024 (1Mbit)

#include <spieeprom.h>

#define PAGE_SIZE        256 // Number of bytes per page  
#define NUM_PAGES        512 // Number of pages in EEPROM  
#define USERS_PER_PAGE    32 // Number of users per memory page
#define MAX_USER_SIZE  16384 // Limit on total number of users

#define TAG_OFFSET_MASK 0x1F  // Number of leftshift bits to obtain 
                              // tag address from masked index
#define TAG_OFFSET_LSHIFT  2  // Number of leftshift bits to obtain 
                              // tag address from masked index
#define AUTH_PAGE_OFFSET 0x80 // Offset on a page where authorisation
                              // bytes begin
#define PAGE2ADDR_LSHIFT   5  // Number of leftshift bits to obtain 
                              // page address from page number
                             
// Start of user count on a page (2 bytes = 65535 users max)
const unsigned int userCountAddr = PAGE_SIZE - 3;

#else // Using builtin Arduino EEPROM

#include <EEPROM.h>

#define NUM_PAGES          1 // Number of pages in EEPROM  
#define MAX_USER_SIZE    247 // Limit on total number of users
#define EEPROM_SIZE     1024
#define NOMINAL_TAG_LEN    4
#define AUTH_OFFSET_SHIFT  3 // Number of rightshift bits to obtain 
                             // authorisation address offset from 
                             // tag address offset

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
    int setAuth(unsigned int tableIndex, byte auth);
    int checkAuthMod(unsigned int tableIndex, byte auth)
    int getAuth(unsigned int tableIndex);
    int getUserIndex(byte *tag_id, int num_bytes = NOMINAL_TAG_LEN);
    
    unsigned int  index2pageAddr(unsigned int tableIndex);
    int index2pageNum(unsigned int tableIndex);
    unsigned long index2tagAddr(unsigned int tableIndex);
    byte index2tagOffset(unsigned int tableIndex);
    unsigned long index2authAddr(unsigned int tableIndex);
    byte index2authMask(unsigned int tableIndex);
    byte index2AuthOffset(unsigned int tableIndex);
    
    int readTag(unsigned long address, byte *tag_id);
    int readTag(unsigned long address, byte *tag_id);
    byte readMemory(unsigned long address);
    void writeToPage(long page_offset, byte value);
    
    void loadUserPage(unsigned int tableIndex);
    void saveUserPage(unsigned int tableIndex);
    
#if defined(EEPROM_SPI) 
    byte _page_data[PAGE_SIZE];
    SPIEEPROM *_spi_eeprom;
#endif  // EEPROM_SPI  
};


#endif // __ACCESSTABLE__
