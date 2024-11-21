#include <RTClib.h>                  // Clock
#include <ezButton.h>                //Encoder Button
#include <LiquidCrystal.h>           // LCD
#include <Adafruit_PWMServoDriver.h> // PWM servo Driver
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C addresses used:
// RTC - 00x50
// PWMs - 00x40/00x41
// LCD - 00x27

/////////////////////////////// Support Variables ///////////////////////////////
// State = Operation Mode
//  1 - Telling time
//  2 - Setting time
//  3 - Testing (not in production)

int state;

// For setting time
int time_setting_state, prev_time_setting_state;
int hour_temp, minute_temp;
int prev_hour_temp, prev_minute_temp;

// For transition functionality
int segments_need_transition_check = 0;

// For mechanical display update functionality
int locations_need_mech_update = 0;
int mech_update_values[4] = {0, 0, 0, 0};

int segment_value_array[4][7] = {
    // Should get an int 0-90
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}};

int curr_segment_value_array[4][7] = {
    // Should get an int 0-90
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}};

// For rotary encoder functionality
int CLK_state, prev_CLK_state;

// For push button functionality
int pb_prev = 0, pb_curr = 0;

// For full minute pass verification
int prev_read_second;

// For servo functionality
int MIN_PULSE_WIDTH = 780;
int MAX_PULSE_WIDTH = 2380;
int PWM_FREQUENCY = 50;

// For 1 second and 0.1 second loop when telling time
unsigned long previous_millis_long_interval = 0;
unsigned long previous_millis_small_interval = 0;
unsigned long init_mech_sequence_millis = 0;

const long long_interval = 1000;         // 1 second interval
const long small_interval = 70;          // 0.07 second interval
const long mech_sequence_interval = 750; // 0.75 second interval

float ratio = 0.6;

/////////////////////////////// Pin Declarations ///////////////////////////////

// Button
int BUTTON_1_PIN = 7;
// Encoder
int CLK_PIN = 17; // A3
int DT_PIN = 16;  // A2
int SW_PIN = 14;  // A0

// Servo Pins- seven pins according to location (0-3)
int pins_arrays[4][7] = {
    // Pin num is 3 chars: BPP
    // B is which pwm board (1 | 2)
    // PP is which pin in the board (00,01,....,15)
    {207, 206, 205, 204, 203, 202, 201},
    {215, 214, 213, 212, 211, 210, 209},
    {107, 106, 105, 104, 103, 102, 101},
    {115, 114, 113, 112, 111, 110, 109}
    };

////////Testing variables
int test_motors_pos = 45;
int num_of_active_digits = 4;
int start_place_of_active_digits = 0;

/////////////////////////////// Setting up Instances ///////////////////////////////
LiquidCrystal_I2C lcd(0x27, 16, 2);
ezButton encoder_button(SW_PIN); // create ezButton object
RTC_DS1307 rtc;
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41); // When I chain board no. 2

//----------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(9600);
  pwm_controllers_setup();
  pinMode_setup();
  encoder_setup();
  rtc_setup();
  lcd.begin();
  lcd.backlight();

  // State setup
  state = 2;              // 1 - Displaying Time, 2 - Setting Time
  time_setting_state = 1; // 1 - minutes, 0 - hours

  states_setup(state, time_setting_state);
}

//----------------------------------------------------------------------------------------
void loop()
{
  // Regardless of mode
  // Handler Functions:

  // Transistion the motors to the desired locations if needed (happens every small interval)
  segment_check_and_transition_handle();

  // if flag for update is raised
  // update digits according to time and values stored
  mechanical_display_handle_updates();

  // - 2 possible states - minute control or hour control

  if (state == 2) ///// Mode: Setting time
  {
    // To detect changes within the loop
    prev_hour_temp = hour_temp;
    prev_minute_temp = minute_temp;
    prev_time_setting_state = time_setting_state;

    // Handle Encoder Press
    handle_enc_press_to_switch_time_setting_state();

    // Manage the 2 states
    if (time_setting_state == 1)
    // Minute Control
    {
      handle_enc_turn_to_modify_counter(minute_temp);
    }
    else
    // Hour Control
    {
      handle_enc_turn_to_modify_counter(hour_temp);
    }
    // Keep time_temp within range (0-59/0-24)
    hour_temp = (hour_temp + 24) % 24;
    minute_temp = (minute_temp + 60) % 60;

    // Detect changes within the loop
    if (prev_hour_temp != hour_temp || prev_minute_temp != minute_temp || prev_time_setting_state != time_setting_state)
    {
      display_placeholder_time(hour_temp, minute_temp);
    }

    // Handle Push Button Press
    if (push_button_was_Pressed())
    {
      // Toggle State
      Serial.println("Toggle State to 1 (Time Display)");
      state = 1;
      rtc.adjust(DateTime(2024, 10, 2, hour_temp, minute_temp, 0));
      delay(100);
      DateTime now = rtc.now();
      delay(100);
      display_time(now);
    }
  }

  else if (state == 1) ///// Mode: Telling time
  {
    // Every round second
    if (millis() - previous_millis_long_interval >= long_interval)
    {
      // Read time
      DateTime now = rtc.now();
      delay(100);

      //  Display update seconds on LCD and serial
      display_second(now);

      // Every round minute
      if (now.second() < 1)
      {
        display_time(now);
      }
      else if (now.second() < 2 && prev_read_second >= 59)
      {
        display_time(now);
      }

      prev_read_second = now.second();

      // reassign second
      previous_millis_long_interval = millis();
    }

    // Handle Push Button Press
    if (push_button_was_Pressed())
    {
      Serial.println("Toggle State to 2 (Time setting)");
      state = 2;
      initialize_setting_time_mode();
    }
  }
  else if (state == 3) ///// Mode: Testing
  {
  }
}

//----------------------- support functions

// With every click of the encoder (CW or CCW) modify the given counter accordingly
void handle_enc_turn_to_modify_counter(int &counter_to_modify)
{
  encoder_button.loop();            // MUST call the loop() function first
  CLK_state = digitalRead(CLK_PIN); // read the current state of the rotary encoder's CLK pin
  // If the state of CLK is changed, then pulse occurred
  // React to only the rising edge (from LOW to HIGH) to avoid double count
  if (CLK_state != prev_CLK_state && CLK_state == HIGH)
  {
    // if the DT state is HIGH
    // the encoder is rotating in clockwise direction => increase the counter
    if (digitalRead(DT_PIN) == HIGH)
    {
      counter_to_modify = counter_to_modify + 1;
    }
    else
    {
      // the encoder is rotating in counter-clockwise direction => decrease the counter
      counter_to_modify = counter_to_modify - 1;
    }
  }

  // save last CLK state
  prev_CLK_state = CLK_state;
}

// With every press of the encoder button switch time setting state
void handle_enc_press_to_switch_time_setting_state()
{
  if (encoder_button.isPressed())
  {
    if (time_setting_state == 0)
    {
      time_setting_state = 1;
    }
    else if (time_setting_state == 1)
    {
      time_setting_state = 0;
    }
    Serial.println("The encoder button is pressed (Toggle time setting state)");
  }
}

// Initaialize Setting Time mode

void initialize_setting_time_mode()
{
  mechanical_display_update_4_digits(-1, -1, -1, -1);
  // - take current time (hours and minutes) and assign them to place holder variables
  DateTime now = rtc.now();
  hour_temp = now.hour();
  minute_temp = now.minute();
  // display them
  display_placeholder_time(hour_temp, minute_temp);
}

// Display Time for time setting mode (LCD display, serial monitor)
void display_placeholder_time(int hour_temp, int minute_temp)
{
  // Calculate digits
  int h1, h2, m1, m2;
  time_to_digits(hour_temp, h1, h2);
  time_to_digits(minute_temp, m1, m2);

  // Display time to LCD
  lcd.setCursor(0, 0);    // move cursor to   (0, 0)
  lcd.print("Setting: "); // print message at (0, 0)
  if (time_setting_state == 1)
  {
    lcd.print("Minutes");
    lcd.setCursor(7, 1);
    lcd.print("<-  ");
    lcd.setCursor(0, 1);
    lcd.print("  ");
  }
  else
  {
    lcd.print("Hours  ");
    lcd.setCursor(0, 1);
    lcd.print("->");
    lcd.setCursor(7, 1);
    lcd.print("    ");
  }

  lcd.setCursor(2, 1);
  String full_time_display_second = String(h1) + String(h2) + ":" + String(m1) + String(m2);
  lcd.print(full_time_display_second);

  // Display to Serial Monitor
  String time_setting_variables = "Time setting |  [is_minute](1/0): " + String(time_setting_state) + "| HH:MM: " + String(h1) + String(h2) + ":" + String(m1) + String(m2);
  Serial.println(time_setting_variables);
}

void digit_display_update(int loc, int digit)
{
  // get segment values with the 1/0 according to dig
  int segment_values[7];
  digit_to_seven_segment(digit, segment_values);

  Serial.println("Digit Location: " + String(loc) + " Digit Value: " + String(digit));
  for (int i = 0; i < 7; i++)
  {
    int val_i = segment_values[i];
    if (val_i > 1) // happens when digit_to_seven_segment had digit out of range
    {
      val_i = test_motors_pos;
    }
    else
    {
      val_i = val_i * 90;
    }

    Serial.print(val_i);
    Serial.print("|");
    segment_values[i] = val_i;
  }
  Serial.println();

  // assign array of 0/90 to the relevant value array according to loc
  if (loc >= 0 && loc <= 3)
  {
    memcpy(segment_value_array[loc], segment_values, sizeof(segment_values));
  }

  segments_need_transition_check = 1;
}

void digit_to_seven_segment(int digit, int sevenSegmentArray[7])
{
  // Define seven-segment display patterns for digits 0 to 9
  int patterns[10][7] = {

      {1, 1, 1, 1, 1, 1, 0}, // 0
      {0, 1, 1, 0, 0, 0, 0}, // 1
      {1, 1, 0, 1, 1, 0, 1}, // 2
      {1, 1, 1, 1, 0, 0, 1}, // 3
      {0, 1, 1, 0, 0, 1, 1}, // 4
      {1, 0, 1, 1, 0, 1, 1}, // 5
      {1, 0, 1, 1, 1, 1, 1}, // 6
      {1, 1, 1, 0, 0, 0, 0}, // 7
      {1, 1, 1, 1, 1, 1, 1}, // 8
      {1, 1, 1, 1, 0, 1, 1}  // 9
  };

  // Check if the input digit is within the valid range
  if (digit >= 0 && digit <= 9)
  {
    // Copy the corresponding seven-segment pattern to the output array
    for (int i = 0; i < 7; i++)
    {
      sevenSegmentArray[i] = patterns[digit][i];
    }
  }
  else
  {
    // If the input digit is out of range, set all elements to 45
    for (int i = 0; i < 7; i++)
    {
      sevenSegmentArray[i] = 2;
    }
  }
}

void move_motor(int motor_pin, int degree)
{
  int pulse_wide, pulse_width;
  int FULL_90_PULSE_WIDTH = MIN_PULSE_WIDTH + (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) * 0.55;

  // Convert degree to pulse width
  pulse_width = map(degree, 0, 90, MIN_PULSE_WIDTH, FULL_90_PULSE_WIDTH);
  pulse_wide = int(float(pulse_width) / 1000000 * PWM_FREQUENCY * 4096);

  int pin = motor_pin % 100;
  int pwm = (motor_pin - pin) / 100;

  // Control Motor
  if (pwm == 1)
  {
    pwm1.setPWM(pin, 0, pulse_wide);
  }
  else if (pwm == 2)
  {
    pwm2.setPWM(pin, 0, pulse_wide);
  }
}

void transition_necesary_segments()
{
  int i_start = start_place_of_active_digits;
  int i_end = start_place_of_active_digits + num_of_active_digits;
  int segments_in_place_count = 7 * (4 - num_of_active_digits);

  for (int i = i_start; i < i_end; i++) // for all digits
  {
    for (int j = 0; j < 7; j++) // for all segments
    {

      int req_value = segment_value_array[i][j];
      int curr_value = curr_segment_value_array[i][j];
      int motor_pin = pins_arrays[i][j];

      if (req_value == curr_value)
      {
        segments_in_place_count = segments_in_place_count + 1;
      }
      else if (abs(req_value - curr_value) > 2) // Move a step forward
      {
        int next_step_value = req_value * (1 - ratio) + curr_value * ratio;
        int step_dif = next_step_value - curr_value;
        if (abs(step_dif) < 2)
        {
          next_step_value = req_value;
        }
        move_motor(motor_pin, next_step_value);
        curr_segment_value_array[i][j] = next_step_value;
      }
      else // Move to required location
      {

        move_motor(motor_pin, req_value);
        curr_segment_value_array[i][j] = req_value;
      }

      // If all segments are in place turn off the flag: segments_need_transition_check

      if (segments_in_place_count == 28)
      {
        Serial.println();
        Serial.println("All segments in place - Time in MILIS: " + String(millis()));
        segments_need_transition_check = 0;
      }
    }
  }
  if (segments_in_place_count < 28)
  {
    Serial.println("Ran 1 Check of segment place: Transistion Necesary - Time in MILIS: " + String(millis()));
  }
}

void states_setup(int state, int time_setting_state)
{
  if (state == 1)
  {
    // Set current time for display
    DateTime now = rtc.now();
    display_time(now);
  }

  if (state == 2)
  {
    initialize_setting_time_mode();
  }
}

void pwm_controllers_setup()
{
  pwm1.begin();
  pwm1.setPWMFreq(PWM_FREQUENCY);
  pwm2.begin();
  pwm2.setPWMFreq(PWM_FREQUENCY);
}

void pinMode_setup()
{
  pinMode(BUTTON_1_PIN, INPUT_PULLUP); // Push Button Pin
}

void encoder_setup()
{
  // configure encoder pins as inputs
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  encoder_button.setDebounceTime(50); // set debounce time to 50 milliseconds

  // read the initial state of the rotary encoder's CLK pin
  prev_CLK_state = digitalRead(CLK_PIN);
}

void rtc_setup()
{
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
}

// Was the push button pressed in the last loop
bool push_button_was_Pressed()
{
  pb_prev = pb_curr;
  pb_curr = digitalRead(BUTTON_1_PIN);
  if (pb_curr == 0 && pb_prev == 1)
  {
    Serial.println("Push Button Pressed!");
    return true;
  }
  else if (pb_curr == 1 && pb_prev == 0)
  {
    return false;
  }
  else
  {
    return false;
  }
}

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
  mechanical_display_update_4_digits(h1, h2, m1, m2);
  print_4_digits(h1, h2, m1, m2);
}

// Runs Mech digit display on 4 given digits
void mechanical_display_update_4_digits(int h1, int h2, int m1, int m2)
{
  Serial.println();
  Serial.println("Mechanical Display Update Sequence initiated");

  locations_need_mech_update = 4;
  mech_update_values[0] = h1;
  mech_update_values[1] = h2;
  mech_update_values[2] = m1;
  mech_update_values[3] = m2;

  // digit_display_update(0, h1);
  // digit_display_update(1, h2);
  // digit_display_update(2, m1);
  // digit_display_update(3, m2);
}

// Pretty Prints to serial monitor the 4 given digits
void print_4_digits(int h1, int h2, int m1, int m2)
{
  String time_print_message = "Time Print [HH:MM]: " + String(h1) + String(h2) + ":" + String(m1) + String(m2);
  Serial.println(time_print_message);
}

// Displays the full time (seconds) on (LCD display, serial monitor)
void display_second(DateTime curr_time)
{
  // Display to Serial Monitor
  if (curr_time.second() % 10 == 0)
  {
    Serial.print(curr_time.second(), DEC);
  }
  else if (curr_time.second() % 5 == 0)
  {
    Serial.print(";");
  }
  else
  {
    Serial.print(".");
  }

  // Display time to LCD

  // Calculate digits
  int h1, h2, m1, m2, s1, s2;
  time_to_digits(curr_time.second(), s1, s2);
  time_to_digits(curr_time.minute(), m1, m2);
  time_to_digits(curr_time.hour(), h1, h2);

  lcd.setCursor(0, 0);            // move cursor to   (0, 0)
  lcd.print("Displaying Time: "); // print message at (0, 0)

  lcd.setCursor(0, 1); // move cursor to second row
  String full_time_display_second = "  " + String(h1) + String(h2) + ":" + String(m1) + String(m2) + ":" + String(s1) + String(s2);
  lcd.print(full_time_display_second);
}

void mechanical_display_handle_updates()
{
  if (locations_need_mech_update > 0)
  {
    int loc_to_update = locations_need_mech_update - 1;
    int time_in_sequence = millis() - init_mech_sequence_millis;

    if (loc_to_update == 3 || time_in_sequence >= mech_sequence_interval)
    {
      digit_display_update(loc_to_update, mech_update_values[loc_to_update]);
      init_mech_sequence_millis = millis();
      locations_need_mech_update = locations_need_mech_update - 1;
      Serial.println("Handler updated digit in location: " + loc_to_update);
    }

    // if(locations_need_mech_update == 0){
    //   Serial.print("Updated 4 digits");
    // }
  }
}

void segment_check_and_transition_handle()
{
  if (millis() - previous_millis_small_interval >= small_interval)
  {
    if (segments_need_transition_check == 1)
    {
      transition_necesary_segments();
    }

    // Update the time for 0.1 s loop
    previous_millis_small_interval = millis();
  }
}
