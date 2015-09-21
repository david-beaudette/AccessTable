/**
 * AccessTable.h
 * 
 * Defines the list of user ID's and associated authorisations.
 **/
 
#ifndef __ACCESSTABLE__
#define __ACCESSTABLE__

#include "Arduino.h"

#include <spieeprom.h>

#define NOMINAL_TAG_LEN    4 // Nominal length of a user tag

#define PAGE_SIZE        256 // Number of bytes per page  
#define NUM_PAGES        512 // Number of pages in EEPROM  
#define USERS_PER_PAGE    32 // Number of users per memory page
#define MAX_USER_SIZE  16384 // Limit on total number of users

#define TAG_OFFSET_MASK 0x1F  // Number of leftshift bits to obtain 
                              // tag address from masked index
#define TAG_OFFSET_RSHIFT  2  // Number of leftshift bits to obtain 
                              // tag address from masked index
#define AUTH_PAGE_OFFSET 0x80 // Offset on a page where authorisation
                              // bytes begin
#define PAGE2ADDR_LSHIFT   8  // Number of leftshift bits to obtain 
                              // page address from page number
                             
// Start of user count on a page (2 bytes = 65535 users max)
const unsigned int userCountAddr = PAGE_SIZE - 2;

class AccessTable {
  public:
    AccessTable(int pin_num);
    unsigned long getNumUsers();    
    int getUserAuth(byte *tag_id, int num_bytes = NOMINAL_TAG_LEN);
    int setUserAuth(byte *tag_id, byte auth);
    int addUser(byte *tag_id, byte auth);
    int clearTable();
    void print_table();
    
  private:
    unsigned int getNumUsersInPage(int pageNum);    
    unsigned int getNumUsersInPageBuffer();    
    int setNumUsers(unsigned int numUsers);
    int setAuth(unsigned int tableIndex, byte auth);
    int checkAuthMod(unsigned int tableIndex, byte auth);
    int getAuth(unsigned int tableIndex);
    int getAuthInPageBuffer(unsigned int userIdx);
    int getUserIndex(byte *tag_id, int num_bytes = NOMINAL_TAG_LEN);
    
    unsigned long index2pageAddr(unsigned int tableIndex);
    unsigned int  index2pageNum(unsigned int tableIndex);
    unsigned long index2tagAddr(unsigned int tableIndex);
    unsigned long index2tagOffset(unsigned int tableIndex);
    unsigned long index2authAddr(unsigned int tableIndex);
    unsigned long index2authMask(unsigned int tableIndex);
    unsigned long index2authOffset(unsigned int tableIndex);
    
    void writeToPageBuf(unsigned int address, byte value);
    
    void loadPage(unsigned int tableIndex);
    void savePage(unsigned int tableIndex);
    void printPageBuffer();
    
    byte _page_buffer[PAGE_SIZE];
    SPIEEPROM *_spi_eeprom;
};


#endif // __ACCESSTABLE__
