/** La Fabrique

   AccessTable class test 
   by David Beaudette
   
   Tests the AccessTable class methods.
   
**/
#define EEPROM_SPI

#include "AccessTable.h"
#include <spieeprom.h>
#include <SPI.h>

// Declare table of users
AccessTable table(19);

// Test configuration options
// Reset EEPROM, clearing previous data 
const int reset_memory = 1;
// Fill table with users and authorisations
const int fill_table = 1;
// Verify authorisation for a few users
const int print_table = 1;
// Verify authorisation for a few users
const int check_auth = 1;

// User tags
const int list_size = 5;
byte tag_list[list_size*4] = { 0xE5, 0x74, 0xCF, 0x28,
                               0x8E, 0xE8, 0xF9, 0x55, 
                               0x91, 0x49, 0xF9, 0x55,
                               0x82, 0xC9, 0xF9, 0x55,
                               0x93, 0xA6, 0xDF, 0xC7};

byte auth_list[list_size] = {1,0,1,0,0};

void setup() {
  // Setup serial communication  
  Serial.begin(115200);
  
  Serial.println(F("AccessTable class test (SPI EEPROM)"));
  Serial.println(F("---------"));  
}

void loop() {
  int ini_num_users;
  int num_users;
  int table_index;
  int test_ok = 1;
  
  // Wait for user input
  Serial.println(F("Send character to begin."));
  while(Serial.available() <= 0) {
    delay(500);
  }

  // Report current table size
  ini_num_users = table.getNumUsers();
  Serial.print(F("Initial number of users is "));
  Serial.println(ini_num_users);
  
  // Clear table
  if(reset_memory) {
    Serial.print(F("Clearing memory..."));
    table.clearTable(); 
    Serial.println(F("done."));
    num_users = table.getNumUsers();
    if(num_users == 0) {
      Serial.println(F("Ok: number of users now 0."));
    }
    else {
      Serial.print(F("Error: reported number of users after clearing table is "));
      Serial.println(num_users);
      test_ok = 0;
    }
    // Check that table is empty
    if(table.getNumUsers() != 0) {
      Serial.println(F("Error: users not 0 after clearing table."));
      test_ok = 0;     
    }   
    if(test_ok) {
      Serial.println(F("Ok: no data found in EEPROM."));
    }
  }
  if(print_table) {
    table.print_table();
  }
  
  if(fill_table) {
    Serial.println(F("Adding users."));
    for(int i = 0; i < list_size; i++) {
      Serial.print(F("Adding user # "));
      Serial.println(i);
      if(table.addUser(&tag_list[4*i], auth_list[i]) != 1) {
        Serial.print(F("Error: user "));
        Serial.print(i);
        Serial.println(F(" already in table."));
      }
      // Check if the number of users is correct
      num_users = table.getNumUsers();
      if(num_users != (i+1)) {
        Serial.print(F("Error: the #users is "));
        Serial.print(num_users);
        Serial.print(F(", "));
        Serial.print(i+1);
        Serial.println(F(" users expected."));
        test_ok = 0;
      }
    }
    if(test_ok) {
      Serial.println(F("Ok: users were added."));
    }
    if(print_table) {
      table.print_table();
    }
  }
  
  // Check if users are found and have the right authorisation
  if(check_auth) {
    num_users = table.getNumUsers();
    for(int i = 0; i < num_users; i++) {
      if(table.getUserAuth(&tag_list[4*i]) != auth_list[i]) {
        Serial.print(F("Error: user # "));
        Serial.print(i);
        Serial.println(F(" have wrong authorisation."));
        test_ok = 0;
      }
    }
    // Check 3 bytes version of the function
    for(int i = 0; i < num_users; i++) {
      if(table.getUserAuth(&tag_list[4*i], 3) != auth_list[i]) {
        Serial.print(F("Error: user # "));
        Serial.print(i);
        Serial.println(F(" have wrong authorisation by comparing 3 bytes."));
        test_ok = 0;
      }
    }
  }
  // Display test conclusion
  if(test_ok) {
    Serial.println(F("Test succeeded."));
  }
  else {
    Serial.println(F("Test failed"));
  }

  // Test complete
  while(1);  
}

