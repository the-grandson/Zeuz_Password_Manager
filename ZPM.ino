#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define MAX_DIGIT 10
#define PIN_DIGITS 4

#define INPUT_PIN_STATE 0
#define VERIFY_PIN_STATE 1
#define SCROLL_ACCOUNTS_STATE 2
#define VIEW_ACCOUNT_STATE 3
uint8_t state = INPUT_PIN_STATE; //FSM state

int PIN_CODE[]={1,2,3,4}; //PIN to unlock device
int PIN_ATTEMPT[]={0,0,0,0};

#define rotary_button 12
#define rotary_gnd 3
#define confirm_button 2

const int NUMBER_OF_ELEMENTS = 10;
const int MAX_SIZE = 12;

#define NUMBER_OF_ACCOUNTS 2
#define MAX_SIZE 12
const char * const accounts [NUMBER_OF_ACCOUNTS] [3] = 
 { {"Google", "zeuz", "1234"},
   {"Facefuckingbookingzing", "luanda", "e basta"}
 };

uint8_t scroll_index = 0;

uint8_t pin_digit = 0; //Actual digit
uint8_t attempt_count = 0; //Failed attempts
uint8_t taps = 0; //Taps per digit

static unsigned long debounce_delay = 50;    // the debounce time; increase if the output flickers

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  pinMode(confirm_button, INPUT_PULLUP);
  pinMode(rotary_button, INPUT_PULLUP);
  pinMode(rotary_gnd, OUTPUT);
  digitalWrite(rotary_gnd, LOW);
  
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();

  testdrawstyles();
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Zeuz Password Manager!"));

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void display_text(const char* header, const char* text) {
  display.clearDisplay();

  display.setTextSize(2);             
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(header);

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.println(text);

  display.display();
}

void display_append_text(const char* text) {
  display.setTextSize(1);             
  display.setTextColor(WHITE);        // Draw white text
  display.print(text);
  display.display();
}

boolean check_pin(){
  for(int i=0; i<PIN_DIGITS; i++){
    if(PIN_CODE[i] != PIN_ATTEMPT[i]){
      return false;
    }
  }
  return true;
}

boolean debounce_btn(unsigned long debounce_delay, int button_pin){

  int last_button_state = HIGH;
  unsigned long last_debounce_time = millis();
  
  while((millis() - last_debounce_time) < debounce_delay){
    int reading = digitalRead(button_pin);
    // If the switch changed due to noise:
    if (reading != last_button_state) {
      // reset the debouncing timer
      last_debounce_time = millis();
    }
    last_button_state = reading;
  }
  if(last_button_state == LOW){
    Serial.println("Button pressed!");
    return true;
  }else{
    return false;
  }
}

void loop() {

  switch (state){
    //Case 0, ask user to input PIN
    case INPUT_PIN_STATE:
      //Display inset PIN
      if(taps == 0){
        display_text("Insert PIN","");
      }
      if(debounce_btn(debounce_delay, rotary_button)){
        // MUST LIMIT number of taps
         taps++;
         if(taps < MAX_DIGIT){
          display_append_text("*");
         }
      }
      if(debounce_btn(debounce_delay+10, confirm_button)){
        PIN_ATTEMPT[pin_digit] = taps;
        pin_digit++;
        state = VERIFY_PIN_STATE;
        Serial.println("GoTo Verify PIN");
      }
      break;
    case VERIFY_PIN_STATE:
      if(pin_digit == PIN_DIGITS){
        Serial.println("Verifying PIN");
        if(check_pin()){
          Serial.println("Correct PIN");
          pin_digit = 0;
          attempt_count = 0;
          taps = 0;
          state = SCROLL_ACCOUNTS_STATE;
          break;
        }else{
          attempt_count ++;
          Serial.println("Pin digit: "+ pin_digit);
          Serial.println("Wrong PIN");
        }
      }else{
        if(pin_digit > MAX_DIGIT){
          //Delete asterisks refresh screen
          attempt_count++;
          pin_digit = 0;
          Serial.println("Wrong PIN");
        }else if (attempt_count >= 3){
          //Display exceeded atempts
          delay(60000);
          attempt_count = 0;
          Serial.println("Wrong PIN");
        }
      }
      state = INPUT_PIN_STATE;
      taps = 0;
      break;
    case SCROLL_ACCOUNTS_STATE:
      if(debounce_btn(debounce_delay+10, rotary_button)){
         scroll_index++;
         if(scroll_index >= NUMBER_OF_ACCOUNTS){
          scroll_index = 0;
         }
         display_text(accounts[scroll_index][0],"");
         Serial.println(accounts[scroll_index][0]);
      }
      if(debounce_btn(debounce_delay+10, confirm_button)){
         Serial.println("Change to show");
         state = VIEW_ACCOUNT_STATE;
      }
      break;
    case VIEW_ACCOUNT_STATE:
      if(debounce_btn(debounce_delay+10, rotary_button)){
         display_text(accounts[scroll_index][1],accounts[scroll_index][2]);
         Serial.println(accounts[scroll_index][1]);
         Serial.println(accounts[scroll_index][2]);
         Keyboard.println(accounts[scroll_index][2]);
         delay(100);
         break;
      }
      if(debounce_btn(debounce_delay+10, confirm_button)){
         state = SCROLL_ACCOUNTS_STATE;
         display_text(accounts[scroll_index][0],"");
         break;
      }
      display_text(accounts[scroll_index][0],accounts[scroll_index][1]);
      break;
  }
}
