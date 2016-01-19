/**
  AccessTable.cpp
  
  Defines the list of user ID's and associated authorisations.
 **/

#include "AccessTable.h"

/**
  SPI EEPROM constructor. Always call print_table or getNumUsers at setup.
  
  @param  pin_num  SPI slave select pin for EEPROM
**/
AccessTable::AccessTable(int pin_num) {
  _spi_eeprom = new SPIEEPROM;
  _spi_eeprom->setup(pin_num);
  _spi_eeprom->protect_none();
  _num_users = 0;
  _num_users_read = 0;
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
  int tableIndex = this->getUserIndex(tag_id, num_bytes);

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
  int tableIndex = this->getUserIndex(tag_id);
 
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
  Add a user and set his authorisation. Does not check
  if user already exists.
  
  @return -1 table full\n
           0 user added to table
**/
int AccessTable::addUser(byte *tag_id, byte auth) {
  // Check for empty entries
  // Serial.print(F("AccessTable::addUser; #users before: "));
  // Serial.println(_num_users);

  if(_num_users >= MAX_USER_SIZE) {
    return -1;
  }
  // New user index is the next free slot
  unsigned long tableIndex = _num_users;
  
  // Load existing page content on memory in page buffer
  this->loadPage(tableIndex);

  // Write authorisation bit
  _page_buffer[this->index2authOffset(tableIndex)] = auth;
  
  // Write user tag
  unsigned long tagOffset = this->index2tagOffset(tableIndex);
  for(int byteNum = 0; byteNum < NOMINAL_TAG_LEN; byteNum++) {
    _page_buffer[tagOffset + byteNum] = tag_id[byteNum];
  }
  // Increase number of users on page
  if(this->getNumUsersInPageBuffer() == 0) {
    // Add first user on page
    _page_buffer[userCountAddr+0] = 1;
    _page_buffer[userCountAddr+1] = 0;
  }
  else if(_page_buffer[userCountAddr] == 0xFF) {
    // LSB full, increment MSB
    _page_buffer[userCountAddr+0] = 0;
    _page_buffer[userCountAddr+1] += 1;
  }
  else {
    // Increment LSB only
    _page_buffer[userCountAddr+0] += 1;
  } 
  // Increment number of users counter
  _num_users++;
  
//this->printPageBuffer();
  // Write page buffer to memory
  this->savePage(tableIndex);
  
  return 0;
}

/**
  Get the number of users in page.
  
  @return number of users
**/
unsigned int AccessTable::getNumUsersInPage(int pageNum) {
  byte userCountBuf[2];
  unsigned int countLSB;
  unsigned int countMSB;
  unsigned int userCount = 0;
  unsigned long addr = pageNum;

  addr = (addr << PAGE2ADDR_LSHIFT) + userCountAddr;
     // Serial.print(F("GetNumUsersInPage: Loading page at address 0x"));
    //Serial.print(addr, HEX);
  _spi_eeprom->read_byte_array(addr, userCountBuf, 2);
    //  Serial.print(F(", content is LSB = "));
    //Serial.print(userCountBuf[0]);
    //  Serial.print(F(", MSB = "));
   // Serial.println(userCountBuf[1]);
  countLSB = userCountBuf[0];
  countMSB = userCountBuf[1];
  // Manage empty pages (empty memory byte is 0xFF)
  if(countLSB < 0xFF && countMSB < 0xFF) {
    userCount = countLSB + (countMSB << 8);
  }
  return userCount;
};

/**
  Get the number of users in page for the page currently loaded in
  page buffer.
  
  @return number of users
**/
unsigned int AccessTable::getNumUsersInPageBuffer() {
  unsigned int countLSB;
  unsigned int countMSB;
  unsigned int userCount = 0;

  countLSB = _page_buffer[userCountAddr+0];
  countMSB = _page_buffer[userCountAddr+1];
  // Manage empty pages (empty memory byte is 0xFF)
  if(countLSB < 0xFF && countMSB < 0xFF) {
    userCount = countLSB + (countMSB << 8);
  }
  return userCount;
};

/**
  Get the number of users in table.
  
  @return number of users
**/
unsigned long AccessTable::getNumUsers() {
  unsigned long userCount = 0;
  if(_num_users_read) {
    userCount = _num_users;
  }
  else {
    unsigned int countInPage;
    for(int i = 0; i < NUM_PAGES; i++) {
      countInPage = this->getNumUsersInPage(i);
      if(countInPage) {
        /*Serial.print(F("There are "));
        Serial.print(countInPage);
        Serial.print(F(" users in page "));
        Serial.println(i);*/
        userCount += countInPage;
      }
    }
    _num_users = userCount;
    _num_users_read = 1;
  }
  return userCount;
};

/**
  Delete all users and authorisations from table.
**/
int AccessTable::clearTable() {
  _spi_eeprom->erase_chip();
  _num_users = 0;
  return 0;
}

/**
  Print all users in table on serial port.
**/
void AccessTable::print_table() {
  byte cur_byte;
  unsigned int numUsers;
  Serial.println(F("Printing access table content."));
  Serial.print(F("There are "));
  Serial.print(this->getNumUsers());
  Serial.println(F(" users registered."));
  // Display users on each memory page
  for(int pageNum = 0; pageNum < NUM_PAGES; pageNum++) {
    Serial.println(F(" "));
    // Load page buffer
    this->loadPage(pageNum);
    Serial.print(F("Page "));
    Serial.print(pageNum);
    Serial.print(F(" has "));
    numUsers = getNumUsersInPageBuffer();
    Serial.print(numUsers);
    Serial.println(F(" users."));
    for(int i = 0; i < numUsers; i++) {
      Serial.print(F("  User "));
      Serial.print(i);
      Serial.print(F(": "));
      for(int j = 0; j < NOMINAL_TAG_LEN; j++) {
        cur_byte = _page_buffer[i*NOMINAL_TAG_LEN + j];
        Serial.print(cur_byte, HEX);
      }
      Serial.print(F(" (auth = "));
      Serial.print(getAuthInPageBuffer(i));
      Serial.println(F(")"));
    }
  } 
  Serial.println(F(" "));
}

/**
  Check if user is authorised from the table index.
  
  @param  tableIndex  index in user table
  
  @return -1 invalid table index\n
           0 user unauthorised\n
           1 user authorised
**/
int AccessTable::getAuth(int tableIndex) {
  if(tableIndex < 0 || tableIndex >= MAX_USER_SIZE) {
    return -1;   
  }
  long address = index2authAddr(tableIndex);
  if(_spi_eeprom->read_byte(address) > 0) {
    return 1;
  }
  else {
    return 0;
  }
}

/**
  Check if user is authorised from the page buffer.
  
  @param  userIdx  user index in page
  
  @return -1 invalid user index\n
           0 user unauthorised\n
           1 user authorised
**/
int AccessTable::getAuthInPageBuffer(unsigned int userIdx) {
  if(userIdx < 0 || userIdx >= USERS_PER_PAGE) {
    return -1;   
  }
  if(_page_buffer[AUTH_PAGE_OFFSET + userIdx] > 0) {
    return 1;
  }
  else {
    return 0;
  }
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
  unsigned long authAddr = this->index2authAddr(tableIndex);
  //Serial.print(F("-- Checking user auth at address "));
  //Serial.print(authAddr);
  byte curAuth = _spi_eeprom->read_byte(authAddr);
  //Serial.print(F(", user auth is currently "));
  //Serial.print(curAuth);
  if(auth == curAuth) {
    // Same authorisation
    //Serial.println(F(", returning 0 (no change)."));
    return 0;
  }
  else {
    // Change authorisation
    //Serial.println(F(", returning 1 (modification)."));
    return 1;
  } 
}

/**
  Set user authorisation. Assumes a check has first been performed.
  
  @param  tableIndex  index in user table
  
  @return 1 authorisation changed in table
**/
int AccessTable::setAuth(unsigned int tableIndex, byte auth) {
  // Write authorisation bit
  _page_buffer[this->index2authOffset(tableIndex)] = auth;
  return 1;  
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
  byte userTag[num_bytes];
  unsigned int tableIndex = -1;
  
  // Find this tag in EEPROM table
  for(curUser = 0; curUser < _num_users; curUser++) {
    _spi_eeprom->read_byte_array(this->index2tagAddr(curUser), userTag, num_bytes);
    byteNum = 0;
    while(userTag[byteNum] == tag_id[byteNum] && \
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
  /*Serial.print(F("AccessTable::index2tagAddr->Index 0x"));
  Serial.print(tableIndex, HEX);
  Serial.print(F(" gives page address 0x"));
  Serial.print(this->index2pageAddr(tableIndex), HEX);
  Serial.print(F(" + tag offset 0x"));
  Serial.println(this->index2tagOffset(tableIndex), HEX);*/
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
         this->index2authOffset(tableIndex);  
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
  unsigned long page_num = this->index2pageNum(tableIndex);
  page_num = page_num << PAGE2ADDR_LSHIFT;
  return page_num;
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
unsigned int AccessTable::index2pageNum(unsigned int tableIndex) {
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
unsigned long AccessTable::index2authMask(unsigned int tableIndex) {
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
unsigned long AccessTable::index2tagOffset(unsigned int tableIndex) {
  unsigned long tag_offset = tableIndex;
  tag_offset &= TAG_OFFSET_MASK;
  // Serial.print(F("AccessTable::index2tagOffset->Index 0x"));
  // Serial.print(tableIndex, HEX);
  // Serial.print(F(" is masked as 0x"));
  // Serial.print(tag_offset, HEX);
  // Serial.print(F(" then l-shifted as 0x"));
  tag_offset = tag_offset >> TAG_OFFSET_RSHIFT;
  // Serial.println(tag_offset, HEX);
  // Serial.print(F(" then mult. by tag length as 0x"));
  tag_offset *= NOMINAL_TAG_LEN;
  // Serial.println(tag_offset, HEX);
  
  return tag_offset;  
};

/**
* @details Computes the offset on a page where a user authorisation 
*          is saved.
* 
* @param   tableIndex   index of user 
* @return  authorisation offset 
**/
unsigned long AccessTable::index2authOffset(unsigned int tableIndex) {
  return AUTH_PAGE_OFFSET + ((tableIndex & TAG_OFFSET_MASK) >> TAG_OFFSET_RSHIFT);  
};

/**
* @details Prints content of the page buffer.
**/
void AccessTable::printPageBuffer() {
  for(int i = 0; i < PAGE_SIZE; i++) {
    Serial.print(F("Page buffer address 0x"));
    Serial.print(i, HEX);
    Serial.print(F(" has byte 0x"));
    Serial.println(_page_buffer[i], HEX);
  }
}

/**
* @details Loads a memory page into page buffer.
* @param   tableIndex   user index or page number to load
**/
void AccessTable::loadPage(unsigned int tableIndex) {
  _spi_eeprom->read_byte_array(this->index2pageAddr(tableIndex), _page_buffer, PAGE_SIZE);
}

/**
* @details Loads a memory page into page buffer.
* @param   tableIndex   user index or page number to load
**/
void AccessTable::savePage(unsigned int tableIndex) {
  _spi_eeprom->write(index2pageAddr(tableIndex), _page_buffer, PAGE_SIZE);
  delay(6);
}
