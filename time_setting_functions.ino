
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
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);    // move cursor to   (0, 0)
    lcd.print("Setting: "); // print message at (0, 0)
    if (time_setting_state == 1)
    {
        lcd.print("Minutes");
        lcd.setCursor(7, 1);
        lcd.print("<-");
        lcd.setCursor(0, 1);
        lcd.print("  ");
    }
    else
    {
        lcd.print("Hours  ");
        lcd.setCursor(0, 1);
        lcd.print("->");
        lcd.setCursor(7, 1);
        lcd.print("  ");
    }

    lcd.setCursor(2, 1);
    lcd.print(h1);
    lcd.print(h2);
    lcd.print(":");
    lcd.print(m1);
    lcd.print(m2);

    // Display to Serial Monitor
    Serial.print("Time setting: [is_minute](1/0) -- [temps] (HH:MM): ");
    Serial.print(time_setting_state);
    Serial.print(" -- ");
    Serial.print(h1);
    Serial.print(h2);
    Serial.print(" : ");
    Serial.print(m1);
    Serial.println(m2);
}