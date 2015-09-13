/**
  AccessTable.cpp
  
  Defines the list of user ID's and associated authorisations.
 **/

#include "AccessTable.h"

/**
  SPI EEPROM constructor.
  
  @param  pin_num  SPI slave select pin for EEPROM
**/
void AccessTable::AccessTable(int pin_num) {
  _spi_eeprom = new SPIEEPROM;
  _spi_eeprom.setup(pin_num);
  _spi_eeprom.protect_none();
}

/**
  Check if user is authorised from its tag ID.
  
  @param  tag_id  user tag (4 bytes)
  
  @return -1 user not found\n
           0 user unauthorised\n
           1 user authorised
**/
int AccessTable::getUserAuth(byte *tag_id, int num_bytes) {
  // Check for user tag in table
  unsigned int tableIndex = this->getUserIndex(tag_id, num_bytes);

  return this->getAuth(tableIndex);
}

/**
  Set user authorisation from its tag ID. 
  
  @param  tag_id  user tag (4 bytes)
  
  @return -1 user not found\n
           0 user authorisation unchanged\n
           1 user authorisation changed
**/
int AccessTable::setUserAuth(byte *tag_id, byte auth) {
  // Check for user tag in table
  unsigned int tableIndex = this->getUserIndex(tag_id);
 
  int authCheckStatus = this->checkAuthMod(tableIndex, auth);
  if(authCheckStatus < 1) {
    return authCheckStatus;
  }
  else {
  // Load existing page content on memory in page buffer
  this->loadPage(tableIndex);
  
  this->setAuth(tableIndex, auth);
  
  // Save page content
  this->savePage(tableIndex);
    
  }
}

/**
  Add a user and set his authorisation.
  
  @return -1 table full\n
           0 user already in table\n
           1 user added to table
**/
int AccessTable::addUser(byte *tag_id, byte auth) {
  // Check if user exists
  if(this->getUserIndex(tag_id) >= 0) {
    return 0;
  }
  // Check for empty entries
  int numUsers = this->getNumUsers();
  if(numUsers >= MAX_USER_SIZE) {
    return -1;
  }
  // New user index is the next free slot
  unsigned int tableIndex = numUsers;
  
  // Load existing page content on memory in page buffer
  this->loadUserPage(tableIndex);

  // Write authorisation bit
  this->setAuth(tableIndex, auth);
  
  // Write user tag
  for(int byteNum = 0; byteNum < NOMINAL_TAG_LEN; byteNum++) {
    EEPROM.write(userStartAddr + tableIndex*NOMINAL_TAG_LEN + byteNum, tag_id[byteNum]);
  }
  // Increase table size (not checking for table full again)
  this->setNumUsers(numUsers + 1); 
  
  // Write page buffer to memory
  this->saveUserPage(tableIndex);
  
  return 1;
}

/**
  Get the number of users in table.
  
  @return number of users
**/
unsigned int AccessTable::getNumUsers() {
  unsigned int countLSB = 0;
  unsigned int countMSB = 0;
  for(int i = 0; i < PAGE_SIZE; i++) {
    countLSB += EEPROM.read(userCountAddr+0);
    countMSB += EEPROM.read(userCountAddr+1);
  }
  return countLSB + (countMSB << 8);
};

/**
  Delete all users and authorisations from table.
**/
int AccessTable::clearTable() {
  _spi_eeprom->erase_chip();
  // Write a 0 to all bytes of the EEPROM
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  return 0;
}

/**
  Print all users in table on serial port.
**/
void AccessTable::print_table() {
  byte cur_byte;
  Serial.println("Printing access table content.");
  Serial.print("There are ");
  Serial.print(this->getNumUsers());
  Serial.println(" users registered:");
  for(int i = 0; i < MAX_USER_SIZE; i++) {
    Serial.print("User ");
    Serial.print(i);
    Serial.print(": ");
    for(int j = 0; j < NOMINAL_TAG_LEN; j++) {
      cur_byte = EEPROM.read(i*NOMINAL_TAG_LEN + j);
      Serial.print(cur_byte, HEX);
      if(j < (NOMINAL_TAG_LEN-1)) {
        Serial.print(", ");
      }
    }
    Serial.print(" (auth = ");
    Serial.print(getAuth(i));
    Serial.println(")");
  } 
  Serial.println(" ");
}

/**
  Check if user is authorised from the table index.
  
  @param  tableIndex  index in user table
  
  @return -1 invalid table index\n
           0 user unauthorised\n
           1 user authorised
**/
int AccessTable::getAuth(unsigned int tableIndex) {
  if(tableIndex < 0 || tableIndex >= MAX_USER_SIZE) {
    return -1;   
  }
  // unsigned int relAddr  = tableIndex >> AUTH_OFFSET_SHIFT;
  // // Serial.print("-- Checking user auth at address ");
  // // Serial.print(authStartAddr + relAddr);
  // byte tableByte = EEPROM.read(authStartAddr + relAddr);
  // // Serial.print(", memory content is 0x");
  // // Serial.print(tableByte, HEX);
  // // Serial.print(", the mask used is 0x");
  // byte byteMask  = 1 << (tableIndex%8);
  // // Serial.print(byteMask, HEX);
  // byte curAuth = tableByte & byteMask;
  // // Serial.print(", the mask result is 0x");
  // // Serial.print(curAuth, HEX);
  // // Serial.print(" -- ");
  
  return (curAuth == byteMask);
}

/**
  Check if user authorisation in memory is different from input.
  
  @param  tableIndex  index in user table
  
  @return -1 invalid table index\n
           0 no change required in table\n
           1 change required in table
**/
int AccessTable::checkAuthMod(unsigned int tableIndex, byte auth) {
  // Check index validity
  if(tableIndex < 0 || tableIndex >= MAX_USER_SIZE) {
    return -1;   
  }
  
  unsigned int relAddr = tableIndex >> 3;
  //Serial.print("-- Checking user auth at address ");
  //Serial.print(authStartAddr + relAddr);
  byte tableByte = EEPROM.read(authStartAddr + relAddr);
  //Serial.print(", memory content is 0x");
  //Serial.print(tableByte, HEX);
  //Serial.print(", the mask used is 0x");
  byte byteMask  = 1 << (tableIndex%8);
  //Serial.print(byteMask, HEX);
  byte curAuth = (tableByte & byteMask) > 0;
  //Serial.print(", user auth is currently ");
  //Serial.print(curAuth);
  if(auth == curAuth) {
    // Same authorisation
    //Serial.println(", returning 0 (no change).");
    return 0;
  }
  else {
    // Change authorisation (bitwise xor used)
    return 1;
  } 
}

/**
  Set user authorisation. Assumes a check has first been performed.
  
  @param  tableIndex  index in user table
  
  @return 1 authorisation changed in table
**/
int AccessTable::setAuth(unsigned int tableIndex, byte auth) {
  unsigned int relAddr = tableIndex >> 3;
  //Serial.print("-- Setting user auth at address ");
  //Serial.print(authStartAddr + relAddr);
  byte tableByte = EEPROM.read(authStartAddr + relAddr);
  //Serial.print(", memory content is 0x");
  //Serial.print(tableByte, HEX);
  //Serial.print(", the mask used is 0x");
  byte byteMask  = 1 << (tableIndex%8);
  //Serial.print(byteMask, HEX);
  byte curAuth = (tableByte & byteMask) > 0;
  //Serial.print(", user auth is currently ");
  //Serial.print(curAuth);
  if(auth == curAuth) {
    // No change
    //Serial.println(", returning 0 (no change).");
    return 0;
  }
  else {
    // Change authorisation (bitwise xor used)
    //Serial.print(", updating the auth byte with 0x");
    tableByte = tableByte ^ byteMask;
    //Serial.print(tableByte, HEX);
    //Serial.println(", returning 1 (authorisation updated).");
    EEPROM.write(authStartAddr + relAddr, tableByte);
    return 1;
  } 
}

/**
  Find index of user in table

  @param  tag_id     user tag (num_bytes bytes)
  @param  num_bytes  tag length to validate in bytes
  
  @return   -1 user not found\n
          >= 0 index of user in table
**/
int AccessTable::getUserIndex(byte *tag_id, int num_bytes) {
  int byteNum;
  int curUser;
  unsigned int tableIndex = -1;
  
  // Find this tag in EEPROM table
  for(curUser = 0; curUser < this->getNumUsers(); curUser++) {
    byteNum = 0;
    while(EEPROM.read(userStartAddr + \
                 curUser*NOMINAL_TAG_LEN + byteNum) == tag_id[byteNum] && \
          byteNum < num_bytes) {
       byteNum++;
    }
    if(byteNum == num_bytes) {
      // All bytes compared equal
      tableIndex = curUser;
      break;
    }
  }     
  return tableIndex;
}

/**
* @details Computes the address where a user tag is saved in memory.
* 
* @param   tableIndex   index of user
* @return  address where user tag is saved
**/
unsigned long AccessTable::index2tagAddr(unsigned int tableIndex) {
  return this->index2pageAddr(tableIndex) + this->index2tagOffset(tableIndex);
};

/**
* @details Computes the address where a user authorisation is saved in memory.
* 
* @param   tableIndex   index of user
* @return  address where user authorisation is saved
**/
unsigned long AccessTable::index2authAddr(unsigned int tableIndex) {
  return this->index2pageAddr(tableIndex) + 
         this->index2authOffset(unsigned int tableIndex);  
};

/**
* @details Computes the start address of the page where a user info 
*          is saved in memory.
* 
* @param   tableIndex   index of user
* @return  address where user info is saved
**/
unsigned long AccessTable::index2pageAddr(unsigned int tableIndex) {
  // Build page address by left shifting the page number
  return this->index2pageNum(tableIndex) << PAGE2ADDR_LSHIFT;
};

/**
* @details Finds the page number where a user info is saved. 
*          Subsequent user indexes are saved on subsequent pages to
*          distribute users across as many pages as possible (minimises
*          number of write cycles).
* 
* @param   tableIndex   index of user
* @return  address where user info is saved
**/
int AccessTable::index2pageNum(unsigned int tableIndex) {
  // Subsequent users on subsequent pages
  return tableIndex%NUM_PAGES;
};

/**
* @details Computes the mask to apply to authorisation byte 
*          in memory to retrieve user authorisation bit.
* 
* @param   tableIndex   index of user
* @return  tag offset
**/
byte AccessTable::index2authMask(unsigned int tableIndex) {
  // For external EEPROM each user has its own authorisation address
  return 1;
};

/**
* @details Computes the offset on a page where a user tag 
*          is saved.
* 
* @param   tableIndex   index of user
* @return  tag offset
**/
byte AccessTable::index2tagOffset(unsigned int tableIndex) {
  return tableIndex * NOMINAL_TAG_LEN;  
};

/**
* @details Computes the offset on a page where a user authorisation 
*          is saved.
* 
* @param   tableIndex   index of user
* @return  authorisation offset 
**/
byte AccessTable::index2authOffset(unsigned int tableIndex) {
  return authStartAddr + (tableIndex >> 3);  
};

/**
* @details Reads a byte from memory.
* 
* @param   address   location to write to
* 
* @return  value read from memory
**/
byte readMemory(unsigned long address) {
  return EEPROM.read(address);
}

/**
* @details Reads a tag from memory.
* 
* @param   address   location to write to
* @param   tag_id    user tag (4 bytes)
* 
* @return  1: success
**/
int readTag(unsigned long address, byte *tag_id) {
  
  return 1;
}

/**
* @details Writes a byte to a page in memory.
* @param   address   location to write to
* @param   value     value to write (0-255)
**/
void writeToPage(unsigned long address, byte value) {
  _page_buffer[address] = value;
}
