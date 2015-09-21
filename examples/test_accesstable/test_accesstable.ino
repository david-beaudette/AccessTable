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
const int list_size = 4;

typedef union tag_t{
  unsigned long long_num;
  byte byte_array[4];
};
tag_t set_tag;
tag_t expected_tag;
byte  set_auth;
byte  expected_auth;

// Timing variables
unsigned long t_start, t_stop, t_sum;
unsigned long t_diff, t_min = 100000, t_max = 0, user_t_max;
float t_avg;

// Function return argument
int status;

void setup() {
  // Setup serial communication  
  Serial.begin(115200);
  
  Serial.println(F("AccessTable class test (SPI EEPROM)"));
  Serial.println(F("---------"));  
}

void loop() {
  unsigned long ini_num_users;
  unsigned long num_users;

  int test_ok = 1;
  
  // Wait for user input
  Serial.println(F("Send character to begin."));
  while(Serial.available() <= 0) {
    delay(500);
  }

  // Report current table size
  t_start = micros();
  ini_num_users = table.getNumUsers();
  t_stop = micros();
  Serial.print(F("Initial number of users is "));
  Serial.println(ini_num_users);
  Serial.print(F("Getting number of users took roughly "));
  t_diff = t_stop - t_start;
  Serial.print(t_diff);
  Serial.println(F(" us."));
  
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
    //table.print_table();
  }
  
  if(fill_table) {
    Serial.println(F("Adding users."));
    // Set values for first user
    set_tag.long_num = 0x55667788;
    set_auth = 1;
    for(int i = 0; i < list_size; i++) {
      Serial.print(F("Adding user # "));
      Serial.println(i);
      t_start = micros();
      status = table.addUser(set_tag.byte_array, set_auth);
      t_stop = micros();
      t_diff = t_stop - t_start;
      t_sum += t_diff;
      if(t_diff < t_min) {
        t_min = t_diff;
      }
      if(t_diff > t_max) {
        t_max = t_diff;
      }
      if(status < 0) {
        Serial.print(F("Error: table was reported full."));
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
      // Increment tag and authorisation values
      set_tag.long_num++;
      set_auth = !set_auth;
    }
    t_avg = t_sum / num_users;
    Serial.print(F("Adding users took roughly "));
    t_diff = t_stop - t_start;
    Serial.print(t_diff);
    Serial.print(F(" us per user (min "));
    Serial.print(t_min);
    Serial.print(F(" us, max "));
    Serial.print(t_max);
    Serial.println(F(" us)."));
    
    if(test_ok) {
      Serial.println(F("Ok: users were added."));
    }
    if(print_table) {
      table.print_table();
    }
  }
  
  // Check if users are found and have the right authorisation
  if(check_auth) {
    // Set values for first user
    expected_tag.long_num = 0x55667788;
    expected_auth = 1;
    num_users = table.getNumUsers();
    for(int i = 0; i < num_users; i++) {
      if(table.getUserAuth(expected_tag.byte_array) != expected_auth) {
        Serial.print(F("Error: user # "));
        Serial.print(i);
        Serial.println(F(" have wrong authorisation."));
        test_ok = 0;
      }
      expected_tag.long_num++;
      expected_auth = !set_auth;
    }
    // Check 3 bytes version of the function
    expected_tag.long_num = 0x55667788;
    expected_auth = 1;
    for(int i = 0; i < num_users; i++) {
      if(table.getUserAuth(expected_tag.byte_array, 3) != expected_auth) {
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

