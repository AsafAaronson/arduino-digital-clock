void digit_display_update(int loc, int digit)
{
  // get segment values with the 1/0 according to dig
  int segment_values[7];
  digit_to_seven_segment(digit, segment_values);

  for (int i = 0; i < 7; i++)
  {
    Serial.println();
    Serial.print("i= ");
    Serial.print(i);
    Serial.print("| value= ");
    Serial.print(segment_values[i] * 90);
    Serial.println();

    segment_values[i] = segment_values[i] * 90;
  }

  // assign to the relevant value array according to loc
  if (loc >= 0 && loc <= 3)
  {
    memcpy(segment_value_array[loc], segment_values, sizeof(segment_values));
  }

  Serial.println("Transistion Necesary");
  Serial.println();
  Serial.print("Time in MILIS: ");
  Serial.print(millis());
  Serial.println();
  segments_need_transition = 1;
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
    // If the input digit is out of range, set all elements to 0
    for (int i = 0; i < 7; i++)
    {
      sevenSegmentArray[i] = 0;
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
  // int segments_in_place_count = 0;
  // for (int i = 0; i < 4; i++)
  int segments_in_place_count = 21;
  for (int i = 3; i < 4; i++)
  {
    for (int j = 0; j < 7; j++)
    {

      int req_value = segment_value_array[i][j];
      int curr_value = curr_segment_value_array[i][j];
      int motor_pin = pins_arrays[i][j];


      if (req_value == curr_value)
      {
        segments_in_place_count = segments_in_place_count + 1;
      }
      else if (abs(req_value - curr_value) > 2)
      {
        // Move a step forward
        int next_step_value = req_value * (1 - ratio) + curr_value * ratio;
        int step_dif = next_step_value - curr_value;
        if (abs(step_dif) < 2)
        {
          // next_step_value = req_value * 0.9 + curr_value * 0.1;
          next_step_value = req_value;
        }
        move_motor(motor_pin, next_step_value);
        curr_segment_value_array[i][j] = next_step_value;
      }
      else
      {
        // Move to required location
        move_motor(motor_pin, req_value);
        curr_segment_value_array[i][j] = req_value;
      }

      // If all segments are in place turn off the flag: segments_need_transition

      if (segments_in_place_count == 28)
      {
        Serial.println("Transistion Done");
        Serial.println();
        Serial.print("Time in MILIS: ");
        Serial.print(millis());
        Serial.println();
        segments_need_transition = 0;


      }
    }
  }
}