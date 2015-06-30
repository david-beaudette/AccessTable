/*
  AccessTable.h
  
  Defines the list of user ID's and associated authorisations.
 */
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

#include "Arduino.h"

#ifdef EEPROM_SPI
// Using external SPI EEPROM 
// Microchip 25LC1024 (1Mbit)
#include <spieeprom.h>

// These constants are relative to 1 page
#define MAX_USER_SIZE     32
#define MAX_EEPROM_SIZE  256
#define NOMINAL_TAG_LEN    4

#else 
// Using builtin Arduino EEPROM
#include <EEPROM.h>

#define MAX_USER_SIZE    247
#define MAX_EEPROM_SIZE 1024
#define NOMINAL_TAG_LEN    4

#endif  // EEPROM_SPI

const unsigned int userStartAddr = 0;
const unsigned int authStartAddr = userStartAddr + NOMINAL_TAG_LEN * MAX_USER_SIZE;
const unsigned int userCountAddr = MAX_EEPROM_SIZE - 3;

class AccessTable {
  public:
    unsigned int getNumUsers();
    int getNumUsers(unsigned int *lsb, unsigned int *msb);
    
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
};


#endif // __ACCESSTABLE__
