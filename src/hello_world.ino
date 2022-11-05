#define LED_PIN 15

#define TEXT ".... . .-.. .-.. --- / .-- --- .-. .-.. -.." // "Hello World" in morse code

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    int i = 0;
    while (TEXT[i] != '\0')
    {
        if (TEXT[i] == '.')
        {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
        else if (TEXT[i] == '-')
        {
            digitalWrite(LED_PIN, HIGH);
            delay(300);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
        else if (TEXT[i] == '/')
        {
            delay(500);
        }
        i++;
    }
    delay(2000);
}