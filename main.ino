#include <RTClib.h>                  // Clock
#include <ezButton.h>                //Encoder Button
#include <LiquidCrystal.h>           // LCD
#include <Adafruit_PWMServoDriver.h> // PWM servo Driver
#include <Wire.h>

// AA
// For VS Code to process and lint properly, uncomment these "include" calls,
// when running the code, it needs to be commented out because the arduino doesn't need these

// #include "time_functions.ino"
// #include "state_functions.ino"
// #include "setup_functions.ino"
// #include "display_functions.ino"
// #include "time_setting_functions.ino"

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
int segments_need_transition = 0;

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

// For servo functionality
int MIN_PULSE_WIDTH = 650;
int MAX_PULSE_WIDTH = 2350;
int PWM_FREQUENCY = 50;

// For 1 second and 0.1 second loop when telling time
unsigned long previous_millis_long_interval = 0;
unsigned long previous_millis_small_interval = 0;
const long long_interval = 1000; // 1 second interval
const long small_interval = 70;  // 0.07 second interval

float ratio = 0.6;

/////////////////////////////// Pin Declarations ///////////////////////////////

// Button
int BUTTON_1_PIN = 17; // A3
// Encoder
int CLK_PIN = 16; // A2
int DT_PIN = 15;  // A1
int SW_PIN = 14;  // A0
// LCD
const int RS = 7, EN = 6, D4 = 5, D5 = 4, D6 = 3, D7 = 2;

// Servo Pins- seven pins according to location (0-3)
int pins_arrays[4][7] = {
    // Pin num is 3 chars: BPP
    // B is which pwm board (1 | 2)
    // PP is which pin in the board (00,01,....,15)
    {99, 99, 99, 99, 99, 99, 99},       // 0
    {99, 99, 99, 99, 99, 99, 99},       // 1
    {99, 99, 99, 99, 99, 99, 99},       // 2
    {200, 212, 103, 108, 112, 208, 106} // 3
};

/////////////////////////////// Setting up Instances ///////////////////////////////
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
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
  lcd.begin(16, 2); // set up number of columns and rows

  // State setup
  state = 1;              // 1 - Displaying Time, 2 - Setting Time
  time_setting_state = 0; // 1 - minutes, 0 - hours

  states_setup(state, time_setting_state);
}

//----------------------------------------------------------------------------------------
void loop()
{
  // Regardless of mode
  // If necesary
  // Transistion the motors to the desired locations (happens every small interval)
  if (millis() - previous_millis_small_interval >= small_interval)
  {

    if (segments_need_transition == 1)
    {
      transition_necesary_segments();
    }

    // Update the time for 0.1 s loop
    previous_millis_small_interval = millis();
  }

  if (state == 2) ///// Mode: Setting time
  {
    // - 2 possible states - minute control or hour control

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
      Serial.println("Toggle State");
      state = 1;
      rtc.adjust(DateTime(2024, 10, 2, hour_temp, minute_temp, 00));
    }
  }

  else if (state == 1) ///// Mode: Telling time
  {
    // Every round second
    if (millis() - previous_millis_long_interval >= long_interval)
    {
      // Read time
      DateTime now = rtc.now();

      //  Display update seconds on LCD
      display_second(now);

      // Every round minute
      if (now.second() < 1)
      {
        display_time(now);
      }

      // reassign second
      previous_millis_long_interval = millis();
    }

    // Handle Push Button Press
    if (push_button_was_Pressed())
    {
      Serial.println("Toggle State to 2 (Time setting)");
      state = 2;
      initialize_setting_time_mode();
      time_display_4_digits(-1, -1, -1, -1);
    }
  }
}
