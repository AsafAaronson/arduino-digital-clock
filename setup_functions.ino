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
    // Replaced This: (Delete if works)
    // // - take current time (hours and minutes) and assign them to place holder variables
    // DateTime now = rtc.now();
    // hour_temp = now.hour();
    // minute_temp = now.minute();
    // // display them
    // display_placeholder_time(hour_temp, minute_temp);
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