// Takes time (0-59) int and returns it as two seperate ints - eg: time = 52 -----> dig_0=5 , dig_1 = 2
void time_to_digits(int time, int &dig_0, int &dig_1)
{ // minutes or hours as int
  dig_1 = time % 10;
  dig_0 = (time - dig_1) / 10;
}

// Main Time display (happens every minute)
// Displays the hours and minutes from the given DateTime obj to (Mech Digit Display, serial print)
void display_time(DateTime curr_time)
{
  int h1, h2, m1, m2;
  // parse time
  time_to_digits(curr_time.hour(), h1, h2);
  time_to_digits(curr_time.minute(), m1, m2);
  // display time
  time_display_4_digits(h1, h2, m1, m2);
  time_print_4_digits(h1, h2, m1, m2);

  Serial.println("Hour");
  Serial.println(curr_time.hour());
  Serial.println("Minute");
  Serial.println(curr_time.minute());
  Serial.println("H1");
  Serial.println(h1);
  Serial.println("H2");
  Serial.println(h2);
  Serial.println("M1");
  Serial.println(m1);
  Serial.println("M2");
  Serial.println(m2);




}

// Runs Mech digit display on 4 given digits
void time_display_4_digits(int h1, int h2, int m1, int m2)
{
  Serial.println("Time Displayed!");
  // Will be:
  // digit_dispay(0,h1);
  // digit_dispay(1,h2);
  // digit_dispay(2,m1);
  digit_display_update(3, m2);
}

// Pretty Prints to serial monitor the 4 given digits
void time_print_4_digits(int h1, int h2, int m1, int m2)
{
  Serial.println("Time Print: ");
  Serial.println("HH:MM");

  Serial.print(h1, DEC);
  Serial.print(h2, DEC);
  Serial.print(':');
  Serial.print(m1, DEC);
  Serial.println(m2, DEC);
}

// Displays the full time (seconds) on (LCD display, serial monitor)
void display_second(DateTime curr_time)
{
  // Display to Serial Monitor
  Serial.print("|");
  Serial.print(curr_time.second(), DEC);

  // Calculate digits
  int h1, h2, m1, m2, s1, s2;
  time_to_digits(curr_time.second(),s1,s2);
  time_to_digits(curr_time.minute(),m1,m2);
  time_to_digits(curr_time.hour(),h1,h2);

  // // Display time to LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);    // move cursor to   (0, 0)
  lcd.print("Displaying Time: "); // print message at (0, 0)
  
  lcd.setCursor(2, 1);
  lcd.print(h1);
  lcd.print(h2);
  lcd.print(":");
  lcd.print(m1);
  lcd.print(m2);
  lcd.print(":");
  lcd.print(s1);
  lcd.print(s2);
}
