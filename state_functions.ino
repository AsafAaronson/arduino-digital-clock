// Was the push button pressed in the last loop
bool push_button_was_Pressed()
{
    pb_prev = pb_curr;
    pb_curr = digitalRead(BUTTON_1_PIN);
    if (pb_curr == 1 && pb_prev == 0)
    {
        Serial.println("Push Button Pressed!");
        return true;
    }
    else if (pb_curr == 0 && pb_prev == 1)
    {
        return false;
    }
    else
    {
        return false;
    }
}