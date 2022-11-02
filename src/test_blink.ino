void setup()
{
    pinMode(D8, OUTPUT); // LED
}

void loop()
{
    digitalWrite(D8, HIGH); // LED on
    delay(100);
    digitalWrite(D8, LOW); // off
    delay(1000);
}
