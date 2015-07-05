/**
  AccessTable.cpp
  
  Defines the list of user ID's and associated authorisations.
 **/

#include "AccessTable.h"

void AccessTable::AccessTable() {
  _spi_eeprom = new SPIEEPROM(1);
  _spi_eeprom.setup(1);
  _spi_eeprom.unprotect();
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

  return this->setAuth(tableIndex, auth);
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
  // Write authorisation bit
  this->setAuth(numUsers, auth);
  
  // Write user tag
  for(int byteNum = 0; byteNum < NOMINAL_TAG_LEN; byteNum++) {
    this->writeMemory(userStartAddr + numUsers*NOMINAL_TAG_LEN + byteNum, tag_id[byteNum]);
  }
  // Increase table size (not checking for table full again)
  this->setNumUsers(numUsers + 1);  
  
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
    countLSB += this->readMemory(userCountAddr+0);
    countMSB += this->readMemory(userCountAddr+1);
  }
  return countLSB + (countMSB << 8);
};

/**
  Delete all users and authorisations from table.
**/
int AccessTable::clearTable() {
#ifdef EEPROM_SPI  
  _spi_eeprom->erase_chip();
#else
  // Write a 0 to all bytes of the EEPROM
  for (int i = 0; i < EEPROM_SIZE; i++) {
    this->writeMemory(i, 0);
  }
  return 0;
#endif  // EEPROM_SPI
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
      cur_byte = this->readMemory(i*NOMINAL_TAG_LEN + j);
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
int AccessTable::getAuth(int tableIndex) {
  if(tableIndex < 0 || tableIndex >= MAX_USER_SIZE) {
    return -1;   
  }
  unsigned int pageAddr = tableIndex >> PAGE_OFFSET_SHIFT;
  unsigned int relAddr  = tableIndex >> AUTH_OFFSET_SHIFT;
  //Serial.print("-- Checking user auth at address ");
  //Serial.print(authStartAddr + relAddr);
  byte tableByte = this->readMemory(authStartAddr + relAddr);
  //Serial.print(", memory content is 0x");
  //Serial.print(tableByte, HEX);
  //Serial.print(", the mask used is 0x");
  byte byteMask  = 1 << (tableIndex%8);
  //Serial.print(byteMask, HEX);
  byte curAuth = tableByte & byteMask;
  //Serial.print(", the mask result is 0x");
  //Serial.print(curAuth, HEX);
  //Serial.print(" -- ");
  
  return (curAuth == byteMask);
}

/**
  Set user authorisation.
  
  @param  tableIndex  index in user table
  
  @return -1 invalid table index\n
           0 no change in table\n
           1 authorisation changed in table
**/
int AccessTable::setAuth(int tableIndex, byte auth) {
  if(tableIndex < 0 || tableIndex >= MAX_USER_SIZE) {
    return -1;   
  }
  unsigned int relAddr = tableIndex >> 3;
  //Serial.print("-- Setting user auth at address ");
  //Serial.print(authStartAddr + relAddr);
  byte tableByte = this->readMemory(authStartAddr + relAddr);
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
    this->writeMemory(authStartAddr + relAddr, tableByte);
    return 1;
  } 
}

/**
  Find index of user in table

  @param  tag_id     user tag (num_bytes bytes)
  @param  num_bytes  tag length to validate in bytes
  
  @return -1 user not found\n
          >0 index of user in table
**/
int AccessTable::getUserIndex(byte *tag_id, int num_bytes) {
  int byteNum;
  int curUser;
  int tableIndex = -1;
  
  // Find this tag in EEPROM table
  for(curUser = 0; curUser < this->getNumUsers(); curUser++) {
    byteNum = 0;
    while(this->readMemory(userStartAddr + \
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
* @details Updates the number of users in table.
* 
* @param   numUsers   number of users
* @return  -1 table full\n
*           0 update successful
**/
int AccessTable::setNumUsers(unsigned int numUsers) {
  if(numUsers > MAX_USER_SIZE) {
    return -1;
  }
  this->writeMemory(userCountAddr+0, numUsers & 0xFF);
  this->writeMemory(userCountAddr+1, (numUsers >> 8) & 0xFF);
  return 0;
};

/**
* @details Reads a byte from memory.
* 
* @param   address   location to write to
* 
* @return  value read from memory
**/
byte readMemory(long address) {
#ifdef EEPROM_SPI  
  return _spi_eeprom->read_byte(address);
#else
  return EEPROM.read(address);
#endif  // EEPROM_SPI

}

/**
* @details Writes a byte to memory.
* @param   address   location to write to
* @param   value     value to write (0-255)
**/
void writeMemory(long address, byte value) {
#ifdef EEPROM_SPI  
  _spi_eeprom->write(address, value);
#else
  EEPROM.write(address, value);
#endif  // EEPROM_SPI
}

/**
* @details Writes a byte array to memory. Only used with 
*          SPI memory to minimise write cycles.
* @param   address   location to write to
* @param   value     values to write 
* @param   length    number of values to write 
**/
void writeMemoryArray(long address, byte *value, int length) {
  _spi_eeprom->write(address, value);
}

