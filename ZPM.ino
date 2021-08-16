#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/* Main aspects to edit are in this code block.
 * ##########################################################################################
 * Define your PIN in the PIN_CODE array.
 */
int PIN_CODE[] = {1,2,3,4}; //YOUR PIN HERE
#define MAX_DIGIT 10

/*
 * Define the NUMBER_OF_ACCOUNTS, MAX_SIZE and fill the accounts array with your credentials.
 */
#define NUMBER_OF_ACCOUNTS 2 //YOUR NUMBER OF CREDENTIALS HERE
#define MAX_SIZE 35 //YOUR MAXIMUM PASSWORD/USERNAME SIZE
const char * const accounts [NUMBER_OF_ACCOUNTS] [3] = 
 { {"Google", "zeuz@zeuz.com", "XF#c?j6hu:.NkNkJ"},
   {"Facefuckingbookingzing", "luanda", "e basta"}
 };
/*
 * ##########################################################################################
 */


const int PIN_DIGITS = sizeof(PIN_CODE)/sizeof(int);
int* PIN_ATTEMPT = 0; //Pointer to the array to store user input

/*
 * The program flow is controlled by a Finite State Machine (FSM).
 * Below the FSM states are defined and the variable that holds the current state is initialized.
 */
#define INPUT_PIN_STATE 0
#define VERIFY_PIN_STATE 1
#define SCROLL_ACCOUNTS_STATE 2
#define VIEW_ACCOUNT_STATE 3
uint8_t state = INPUT_PIN_STATE; //Current FSM state

/*
 * Definition of the buttons.
 * 
 * ATTENTION: Made a big mistake and coud not chage the routing of the ROTARY button to a GND pin.
 * Because of that added a 1kOhm resistor and used ROTARY_GND pin as a GND pin.
 * If you are willing to make this properly change this in your making process.
 */
#define ROTARY_BTN 12
#define ROTARY_GND 3
#define CONFIRM_BTN 2


uint8_t scroll_index = 0;

uint8_t pin_digit = 0; //Current digit
uint8_t attempt_count = 0; //Failed attempts
uint8_t taps = 0; //Taps per digit

static unsigned long debounce_delay = 50;    // the debounce time; increase if the output flickers

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int display_x, min_display_x = 0;

void setup() {

  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ROTARY_BTN, INPUT_PULLUP);
  pinMode(ROTARY_GND, OUTPUT);
  digitalWrite(ROTARY_GND, LOW);

  PIN_ATTEMPT = new int[PIN_DIGITS];  //Initialize PIN input array
  
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();
  display.setTextWrap(false);
  display_x = 0;
  min_display_x = -12 * MAX_SIZE;  // 12 = 6 pixels/character * text size 2
  display_start();
}

void display_start(void) {
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
  /*display.clearDisplay();

  display.setTextSize(2);             
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(header);

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.println(text);

  display.display();*/
   display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(0,0);
   display.setTextSize(2);
   display.print(header);// GPS # Satellites, Time, % Batt chg
   display.setTextSize(1);
   display.setCursor(display_x,16);
   display.print(text);
   display.display();
   Serial.println(strlen(text));
   /*
    * Code block to make the display scroll adapted from J Doe answer in the following StackOverflow link:
    * https://stackoverflow.com/questions/40564050/scroll-long-text-on-oled-display
    */
   if(strlen(text) > 21){
     display_x=display_x-3; // scroll speed, make more positive to slow down the scroll
     if(display_x < min_display_x) display_x = display.width();
   }
}

void display_header_and_lines(const char* header, const char* line1, const char* line2) {
  /*display.clearDisplay();

  display.setTextSize(2);             
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(header);

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.println(text);

  display.display();*/
   display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(0,0);
   display.setTextSize(2);
   display.print(header);// GPS # Satellites, Time, % Batt chg
   display.setTextSize(1);
   display.setCursor(0,16);
   display.print(line1);
   display.setCursor(display_x,25);
   display.print(line2);
   display.display();
   if(strlen(line2)){
     display_x=display_x-2; // scroll speed, make more positive to slow down the scroll
     if(display_x < min_display_x) display_x = display.width();
   }
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
      if(debounce_btn(debounce_delay, ROTARY_BTN)){
        // MUST LIMIT number of taps
         taps++;
         if(taps < MAX_DIGIT){
          display_append_text("*");
         }
      }
      if(debounce_btn(debounce_delay+10, CONFIRM_BTN)){
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
      if(debounce_btn(debounce_delay+10, ROTARY_BTN)){
         scroll_index++;
         if(scroll_index >= NUMBER_OF_ACCOUNTS){
          scroll_index = 0;
         }
         display_text(accounts[scroll_index][0],"");
         Serial.println(accounts[scroll_index][0]);
      }
      if(debounce_btn(debounce_delay+10, CONFIRM_BTN)){
         Serial.println("Change to show");
         min_display_x = -6 * strlen(accounts[scroll_index][2]); //Each time use the password size to set the new scroll limit.
         state = VIEW_ACCOUNT_STATE;         
      }
      break;
    case VIEW_ACCOUNT_STATE:
      if(debounce_btn(debounce_delay+10, ROTARY_BTN)){
         display_header_and_lines(accounts[scroll_index][0],accounts[scroll_index][1],accounts[scroll_index][2]);
         Serial.println(accounts[scroll_index][1]);
         Serial.println(accounts[scroll_index][2]);
         delay(100);
         break;
      }
      if(debounce_btn(debounce_delay+10, CONFIRM_BTN)){
         state = SCROLL_ACCOUNTS_STATE;
         display_x = 0;
         display_text(accounts[scroll_index][0],"");
         break;
      }
      display_text(accounts[scroll_index][0],accounts[scroll_index][1]);
      break;
  }
}
