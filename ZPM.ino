#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


/* ##########################################################################################
 *  Main aspects to edit are in this code block.
 * ##########################################################################################
 * Define your PIN in the PIN_CODE array.
 */
int PIN_CODE[] = {1,2,3,4}; //YOUR PIN HERE
#define MAX_DIGIT 10 //Define the maximum integer number each PIN position can be.

/*
 * Define the NUMBER_OF_ACCOUNTS, MAX_SIZE and fill the accounts array with your credentials.
 */
#define NUMBER_OF_ACCOUNTS 3 //YOUR NUMBER OF CREDENTIALS HERE
#define MAX_SIZE 35 //YOUR MAXIMUM PASSWORD/USERNAME SIZE
const char * const accounts [NUMBER_OF_ACCOUNTS] [3] = 
 { {"Google", "zeuz@zeuz.com", "XF#c?j6hu:.NkNkJ"},
   {"FB", "luanda", "e basta"},
   {"GitHub", "zeuz", "hzt*7!HZU\"dApP'tjc;M^FW?mr^m"}
 };
/*
 * ##########################################################################################
 */

/*
 * Uncomment the DEBUG definition if you want to print debug messages over the serial port.
 */
//#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
#endif

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
#define WAIT_STATE 4
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
const unsigned long debounce_delay = 50;    //Button debounce time, increase it if the output flickers

uint8_t scroll_index = 0; //Keep track of the currently selected account
uint8_t pin_digit = 0; //Current input PIN digit
uint8_t attempt_count = 0; //Failed attempts
uint8_t taps = 0; //Taps per digit

unsigned long view_timeout_millis; //Keep track of time the device is unclocked
const unsigned long TIMEOUT_MILLIS = 60000; //Change this to increase/decrease the timeout to lock the device

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int display_x, min_display_x = 0;
const int MAX_CHARS_PER_LINE = display.width()/6; // 6 pixels/character

void setup() {
  delay(1000);
  
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ROTARY_BTN, INPUT_PULLUP);
  pinMode(ROTARY_GND, OUTPUT);
  digitalWrite(ROTARY_GND, LOW);

  PIN_ATTEMPT = new int[PIN_DIGITS];  //Initialize PIN input array

#ifdef DEBUG
  Serial.begin(9600);
#endif
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    DEBUG_PRINT("SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();
  display.setTextWrap(false);
  display_x = 0;
  min_display_x = -6 * MAX_SIZE;  // 6 pixels/character x max length
  display_start();
}

void display_start(void) {
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println("Zeuz Password Manager!");
  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.display();
  delay(2000);
}

void display_text(const char* header, const char* text) {
  /*
  * Code block to make the display scroll adapted from J Doe answer in the following StackOverflow link:
  * https://stackoverflow.com/questions/40564050/scroll-long-text-on-oled-display
  */
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextSize(2);
  display.print(header);
  display.setTextSize(1);
  display.setCursor(0,16);
  display.print(text);
  display.display();
  
  if(strlen(text) > MAX_CHARS_PER_LINE){
   display_x=display_x-3; // scroll speed, make more positive to slow down the scroll
   if(display_x < min_display_x) display_x = display.width();
  }
}

void display_header_and_lines(const char* header, const char* line1, const char* line2) {
  
   display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(0,0);
   display.setTextSize(2);
   display.print(header);
   display.setTextSize(1);
   display.setCursor(0,16);
   display.print(line1);
   display.setCursor(display_x,25);
   display.print(line2);
   display.display();
   if(strlen(line2) > MAX_CHARS_PER_LINE){
     display_x=display_x-3; // scroll speed, make more positive to slow down the scroll
     if(display_x < min_display_x) display_x = display.width();
   }
}

void display_append_text(const char* text) {
  display.setTextSize(1);             
  display.setTextColor(WHITE);
  display.print(text);
  display.display();
}

boolean check_timeout(void){
  if(view_timeout_millis < millis()){
    state = INPUT_PIN_STATE;
    display_x = 0;
    return true;
  }
  return false;
}

void reset_timeout(void){
  view_timeout_millis = millis() + TIMEOUT_MILLIS;
  DEBUG_PRINT("Timeout reset!");
}

boolean check_pin(void){
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
    DEBUG_PRINT("Button pressed!");
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
         }else{
          taps = 0;
         }
      }
      if(debounce_btn(debounce_delay+10, CONFIRM_BTN)){
        PIN_ATTEMPT[pin_digit] = taps;
        pin_digit++;
        state = VERIFY_PIN_STATE;
        DEBUG_PRINT("GoTo Verify PIN");
      }
      break;
    case VERIFY_PIN_STATE:
      if(pin_digit >= PIN_DIGITS){
        DEBUG_PRINT("Verifying PIN");
        if(check_pin()){
          DEBUG_PRINT("Correct PIN");
          pin_digit = 0;
          attempt_count = 0;
          taps = 0;
          state = SCROLL_ACCOUNTS_STATE;
          display_text(accounts[scroll_index][0],"");
          reset_timeout();
          break;
        }else{
          attempt_count ++;
          pin_digit = 0;
          DEBUG_PRINT("Wrong PIN");
          if (attempt_count >= 3){
            state = WAIT_STATE;
            taps = 0;
            break;
          }
        }
      }
      state = INPUT_PIN_STATE;
      taps = 0;
      break;
    case WAIT_STATE:
      display_header_and_lines("","Exceeded Attempts!","Wait 60s.");
      delay(60000);
      state = INPUT_PIN_STATE;
      attempt_count = 0;
      break;
    case SCROLL_ACCOUNTS_STATE:
      if(check_timeout()){
        break;
      }
      if(debounce_btn(debounce_delay+10, ROTARY_BTN)){
         scroll_index++;
         if(scroll_index >= NUMBER_OF_ACCOUNTS){
          scroll_index = 0;
         }
         display_text(accounts[scroll_index][0],"");
         DEBUG_PRINT(accounts[scroll_index][0]);
      }
      if(debounce_btn(debounce_delay+10, CONFIRM_BTN)){
         DEBUG_PRINT("Change to show");
         min_display_x = -6 * strlen(accounts[scroll_index][2]); //Each time use the password size to set the new scroll limit.
         state = VIEW_ACCOUNT_STATE;
         reset_timeout();     
      }
      break;
    case VIEW_ACCOUNT_STATE:
      if(check_timeout()){
        break;
      }
      if(debounce_btn(debounce_delay+10, ROTARY_BTN)){
         display_header_and_lines(accounts[scroll_index][0],accounts[scroll_index][1],accounts[scroll_index][2]);
         DEBUG_PRINT(accounts[scroll_index][1]);
         DEBUG_PRINT(accounts[scroll_index][2]);
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
