/*
  AccessTable.h
  
  Defines the list of user ID's and associated authorisations.
 */
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

#include "Arduino.h"
#include <EEPROM.h>

#define MAX_USER_SIZE    248
#define MAX_EEPROM_SIZE 1024

const unsigned int userStartAddr = 0;
const unsigned int authStartAddr = userStartAddr + 4 * MAX_USER_SIZE;
const unsigned int userCountAddr = MAX_EEPROM_SIZE - 2;

class AccessTable {
  public:
    unsigned int getNumUsers();
    int getNumUsers(unsigned int *lsb, unsigned int *msb);
    
    int getUserAuth(byte *tag_id);
    int setUserAuth(byte *tag_id, byte auth);
    int addUser(byte *tag_id, byte auth);
    int clearTable();
    void print_table();
    
  private:
    int setNumUsers(unsigned int numUsers);
    int setAuth(int tableIndex, byte auth);
    int getAuth(int tableIndex);
    int getUserIndex(byte *tag_id);
};


#endif // __ACCESSTABLE__
