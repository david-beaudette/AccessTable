/*
  AccessTable.cpp
  
  Defines the list of user ID's and associated authorisations.
 */

#include "AccessTable.h"

/*
  Check if user is authorized from its tag ID.
  
  @param  tag_id  user tag (4 bytes)
  
  @return -1 invalid table index\n
           0 user unauthorized\n
           1 user authorized
*/
int AccessTable::getUserAuth(byte *tag_id) {
  // Check for user tag in table
  int tableIndex = this->getUserIndex(tag_id);

  return this->getAuth(tableIndex);
}

/*
  Add a user and set his authorization.
  
  @return -1 table full\n
           0 user already in table\n
           1 user added to table
*/
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
  // Write authorization bit
  this->setAuth(numUsers, auth);
  
  // Write user tag
  for(int byteNum = 0; byteNum < 4; byteNum++) {
    EEPROM.write(userStartAddr + numUsers*4 + byteNum, tag_id[byteNum]);
  }
  // Increase table size (not checking for table full again)
  this->setNumUsers(numUsers + 1);  
  
  return 1;
}

/*
  Get the number of users in table.
  
  @return number of users
*/
unsigned int AccessTable::getNumUsers() {
  unsigned int countLSB = EEPROM.read(userCountAddr+0);
  unsigned int countMSB = EEPROM.read(userCountAddr+1);
  return countLSB + (countMSB << 8);
};

/*
  Delete all users and authorizations from table.
*/
int AccessTable::clearTable() {
  // Write a 0 to all bytes of the EEPROM
  for (int i = 0; i < MAX_EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  return 0;
}

/*
  Print all users in table on serial port.
*/
void AccessTable::print_table() {
  byte cur_byte;
  Serial.println("Printing access table content.");
  Serial.println("Users:");
  for(int i = 0; i < MAX_USER_SIZE; i++) {
    Serial.print("User ");
    Serial.print(i);
    Serial.print(": ");
    for(int j = 0; j < 4; j++) {
      cur_byte = EEPROM.read(i*4 + j);
      Serial.print(cur_byte, HEX);
      if(j < 3) {
        Serial.print(", ");
      }
    }
    Serial.print(" (auth = ");
    Serial.print(getAuth(i));
    Serial.println(")");
  }   
}

/*
  Check if user is authorized from the table index.
  
  @param  tableIndex  index in user table
  
  @return -1 invalid table index\n
           0 user unauthorized\n
           1 user authorized
*/
int AccessTable::getAuth(int tableIndex) {
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
  byte curAuth = tableByte & byteMask;
  //Serial.print(", the mask result is 0x");
  //Serial.print(curAuth, HEX);
  //Serial.print(" -- ");
  
  return (curAuth == byteMask);
}

/*
  Set user authorization.
  
  @param  tableIndex  index in user table
  
  @return -1 invalid table index\n
           0 no change in table\n
           1 authorization changed in table
*/
int AccessTable::setAuth(int tableIndex, byte auth) {
  if(tableIndex < 0 || tableIndex >= MAX_USER_SIZE) {
    return -1;   
  }
  unsigned int relAddr = tableIndex >> 3;
  Serial.print("-- Setting user auth at address ");
  Serial.print(authStartAddr + relAddr);
  byte tableByte = EEPROM.read(authStartAddr + relAddr);
  Serial.print(", memory content is 0x");
  Serial.print(tableByte, HEX);
  Serial.print(", the mask used is 0x");
  byte byteMask  = 1 << (tableIndex%8);
  Serial.print(byteMask, HEX);
  byte curAuth = (tableByte & byteMask) > 0;
  Serial.print(", user auth is currently ");
  Serial.print(curAuth);
  if(auth == curAuth) {
    // No change
    Serial.println(", returning 0 (no change).");
    return 0;
  }
  else {
    // Change authorization (bitwise xor used)
    Serial.print(", updating the auth byte with 0x");
    tableByte = tableByte ^ byteMask;
    Serial.print(tableByte, HEX);
    Serial.println(", returning 1 (authorization updated).");
    EEPROM.write(authStartAddr + relAddr, tableByte);
    return 1;
  } 
}

/*
  Find index of user in table

  @param  tag_id  user tag (4 bytes)
  
  @return -1 user not found\n
          >0 index of user in table
*/
int AccessTable::getUserIndex(byte *tag_id) {
  int byteNum;
  int curUser;
  int tableIndex = -1;
  
  // Find this tag in EEPROM table
  for(curUser = 0; curUser < this->getNumUsers(); curUser++) {
    byteNum = 0;
    while(EEPROM.read(userStartAddr + curUser*4 + byteNum) == tag_id[byteNum] && \
          byteNum < 4) {
       byteNum++;
    }
    if(byteNum == 4) {
      // All bytes compared equal
      tableIndex = curUser;
      break;
    }
  }     
  return tableIndex;
}

int AccessTable::setNumUsers(unsigned int numUsers) {
  if(numUsers > MAX_USER_SIZE) {
    return -1;
  }
  EEPROM.write(userCountAddr+0, numUsers & 0xFF);
  EEPROM.write(userCountAddr+1, (numUsers >> 8) & 0xFF);
  return 0;
};
