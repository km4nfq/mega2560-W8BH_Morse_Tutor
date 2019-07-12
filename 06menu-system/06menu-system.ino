/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   26 Jun 2019
    Hardware:   STM32F103C "Blue Pill", Piezo Buzzer, 2.2" ILI9341 LCD,
                Adafruit #477 rotary encoder or similar
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
    
 Description:   Morse Code Tutor, Part 6
                Builds on Part 5, adding a menu system
   
 **************************************************************************/
/**************************************************************************
Modification: Ken, KM4NFQ "Not Fully Qualified"
        Date: 9 Jul 2019
    Hardware: RobotDyn Mega2560 Pro Mini (non-USB version)
              programmed with a USBtoSerial Adapter
              ILI9341 2.2" TFT SPI 320x240 display
              Small speaker, RobotDyn Rotary Encoder module
    Software: Arduino IDE 1.8.9
 Description: 'port' of Morse Tutor Tutorial Part 6 from Blu Pill
              to a Mega2560 Pro Mini - adding a menu system
 **************************************************************************/

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define CODESPEED          13         // speed in Words per Minute
#define PITCH       700               // CHG pitch in Hz of morse audio
#define LED         13                // CHG onboard LED pin
#define PIEZO       26                // CHG pin attached to piezo element
#define DITPERIOD   1200/CODESPEED    // period of dit, in milliseconds
#define WORDSIZE          5           // number of chars per random word
#define PADDLE_A    33                // CHG Morse Paddle "dit"
#define PADDLE_B    35                // CHG Morse Paddle "dah"
#define ENCODER_A         2           // CHG Rotary Encoder output A
#define ENCODER_B         3           // CHG Rotary Encoder output B
#define ENCODER_BUTTON   18           // CHG Rotary Encoder switch
#define ENCODER_TICKS     3           // Ticks required to register movement
#define TFT_DC      48                // CHG for direct port access
#define TFT_CS      47                // CHG for direct port access
#define TFT_RST     44                // CHG

//===================================  Color Constants ==================================
#define BLACK          0x0000
#define BLUE           0x001F
#define RED            0xF800
#define GREEN          0x07E0
#define CYAN           0x07FF
#define MAGENTA        0xF81F
#define YELLOW         0xFFE0
#define WHITE          0xFFFF
#define DKGREEN        0x03E0

// ==================================  Menu Constants ===================================
#define DISPLAYWIDTH      320        // Number of LCD pixels in long-axis
#define DISPLAYHEIGHT     240        // Number of LCD pixels in short-axis
#define TOPDEADSPACE       30        // All submenus appear below top line
#define MENUSPACING       100        // Width in pixels for each menu column
#define ROWSPACING         25        // Height in pixels for each text row
#define COLSPACING         12        // Width in pixels for each text character
#define MAXCOL   DISPLAYWIDTH/COLSPACING // Number of characters per row 
#define MAXROW  (DISPLAYHEIGHT-TOPDEADSPACE)/ROWSPACING  // Number of text-rows per screen
#define FG              GREEN        // Menu foreground color 
#define BG              BLACK        // Menu background color
#define SELECTFG         BLUE        // Selected Menu foreground color
#define SELECTBG        WHITE        // Selected Menu background color
#define TEXTCOLOR      YELLOW        // Default non-menu text color
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))    // Handy macro for determining array sizes

//===================================  Rotary Encoder Variables =========================
volatile int      rotary_counter   = 0;     // "position" of rotary encoder (increments CW) 
volatile boolean  rotary_changed   = false; // true if rotary_counter has changed
volatile boolean  button_pressed   = false; // true if the button has been pushed
volatile boolean  button_released  = false; // true if the button has been released (sets button_downtime)
volatile long     button_downtime  = 0L;    // ms the button was pushed before released

//===================================  Menu Variables ===================================
int  menuCol=0, textRow=0, textCol=0;
char *mainMenu[] = {" Receive ", "  Send   ", "  Config "};        
char *menu0[]    = {" Letters ", " Numbers ", " Punc    ", " Words   ", " Let-Num ", "Call Sign", " QSO     ", " Exit    "};
char *menu1[]    = {" Practice", " CopyCat ", "Flashcard", " Exit    "};
char *menu2[]    = {" Speed   ", "CharSpeed", " Chk Spd ", " Tone    ", " Dit Pad ", " Defaults", " Exit    "};


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

char *commonWords = "the of and to a in that is was he for it with as his on be at by I this had not are but from or have an they which one you were all her she there would their we him been has when who will no more if out so up said what its about than into them can only other time new some could these two may first then do any like my now over such our man me even most made after also did many off before must well back through years much where your way"; // CHG
char *hamWords[]  = {"DE", "TNX FER", "BT", "WX", "HR", "TEMP", "ES", "RIG", "ANT", "DIPOLE", "VERTICAL", // 0-10
                    "BEAM", "HW", "CPI", "WARM", "SUNNY", "CLOUDY", "COLD", "RAIN", "SNOW", "FOG",       // 11-20
                    "RPT", "NAME", "QTH", "CALL", "UR", "SLD", "FB", "RST"                               // 21-28
                    };
char prefix[]     = {'A', 'W', 'K', 'N'};

const byte morse[] = {                 // Each character is encoded into an 8-bit byte:
  0b01001010,        // ! exclamation  // Some punctuation not commonly used
  0b01101101,        // " quotation    // from wikipedia: Intl Morse Code
  0b0,               // # pound
  0b10110111,        // $ dollar
  0b00000000,        // % percent
  0b00111101,        // & ampersand
  0b01100001,        // ' apostrophe
  0b00110010,        // ( open paren
  0b01010010,        // ) close paren
  0b0,               // * asterisk
  0b00110101,        // + plus or ~AR
  0b01001100,        // , comma
  0b01011110,        // - hypen
  0b01010101,        // . period
  0b00110110,        // / slant   
  0b00100000,        // 0       // Read the bits from RIGHT to left, 
  0b00100001,        // 1       // with a "1"=dit and "0"=dah
  0b00100011,        // 2       // example: 2 = 11000 or dit-dit-dah-dah-dah
  0b00100111,        // 3       // the final bit is always 1 = stop bit.
  0b00101111,        // 4       // see "sendElements" routine for more info.
  0b00111111,        // 5
  0b00111110,        // 6
  0b00111100,        // 7
  0b00111000,        // 8
  0b00110000,        // 9
  0b01111000,        // : colon
  0b01101010,        // ; semicolon
  0b0,               // <
  0b00101110,        // = equals or ~BT
  0b0,               // >
  0b01110011,        // ? question
  0b01101001,        // @ at
  0b00000101,        // A 
  0b00011110,        // B
  0b00011010,        // C
  0b00001110,        // D
  0b00000011,        // E
  0b00011011,        // F
  0b00001100,        // G
  0b00011111,        // H
  0b00000111,        // I
  0b00010001,        // J
  0b00001010,        // K
  0b00011101,        // L
  0b00000100,        // M
  0b00000110,        // N
  0b00001000,        // O
  0b00011001,        // P
  0b00010100,        // Q
  0b00001101,        // R
  0b00001111,        // S
  0b00000010,        // T
  0b00001011,        // U
  0b00010111,        // V
  0b00001001,        // W
  0b00010110,        // X
  0b00010010,        // Y 
  0b00011100         // Z
};

void ditSpaces(int spaces=1) {               // user specifies #dits to wait
  for (int i=0; i<spaces; i++)               // count the dits...
    delay(DITPERIOD);                        // no action, just mark time
}

void wordSpace() {
  ditSpaces(4);                              // 7 total (3 in char)
}

void characterSpace() {
  ditSpaces(2);                              // 3 total (1 in last element)
}


void dit() {
  digitalWrite(LED,1);                // CHG turn on LED
  tone(PIEZO,PITCH);                  // and turn on sound
  ditSpaces();                        // wait for period of 1 dit
  digitalWrite(LED,0);                // CHG turn off LED
  noTone(PIEZO);                      // and turn off sound
  ditSpaces();                        // space between code elements
}

void dah() {
  digitalWrite(LED,1);                // CHG turn on LED
  tone(PIEZO,PITCH);                  // and turn on sound
  ditSpaces(3);                       // length of dah = 3 dits
  digitalWrite(LED,0);                // CHG turn off LED
  noTone(PIEZO);                      // and turn off sound
  ditSpaces();                        // space between code elements
}

/* The following routine accepts a numberic input, where each bit
   represents a code element: 1=dit and 0=dah.   The elements are
   read right-to-left.  The left-most bit is a stop bit.
   For example:  5 = binary 0b0101.  The right-most bit is 1 so 
   the first element is a dit.
   The next element to the left is a 0, so a dah follows.  Only a 
   1 remains after that, so the character is complete: 
   dit-dah = morse character 'A'.*/

void sendElements(int x) {           // send string of bits as Morse      
  while (x>1) {                      // stop when value is 1 (or less)
    if (x & 1) dit();                // right-most bit is 1, so dit
    else dah();                      // right-most bit is 0, so dah
    x >>= 1;                         // rotate bits right
  }
  characterSpace();                  // add inter-character spacing
}

void sendCharacter(char c) {         // send one ASCII character in Morse
  if (c<32) return;                  // ignore control characters
  if (c>96) c -= 32;                 // convert lower case to upper case
  if (c>90) return;                  // not a character
  addCharacter(c);                   // display character on LCD
  if (c==32) wordSpace();            // space between words 
  else sendElements(morse[c-33]);    // send the character
}

void sendString (char *ptr) {             
  while (*ptr)                       // send the entire string
    sendCharacter(*ptr++);           // one character at a time
}

void addChar (char* str, char ch)    // adds 1 character to end of string
{                                            
  char c[2] = " ";                   // happy hacking: char into string
  c[0] = ch;                         // change char'A' to string"A"
  strcat(str,c);                     // and add it to end of string
}

char randomLetter()                  // returns a random uppercase letter
{
  return 'A'+random(0,26);
}

char randomNumber()                  // returns a random single-digit # 0-9
{
  return '0'+random(0,10);
}


void createCallsign(char* call)      // returns random US callsign in "call"
{  
  strcpy(call,"");                   // start with empty callsign                      
  int i = random(0, 4);              // 4 possible start letters for US                                       
  char c = prefix[i];                // Get first letter of prefix
  addChar(call,c);                   // and add it to callsign
  i = random(0,3);                   // single or double-letter prefix?
  if ((i==2) or (c=='A'))            // do a double-letter prefix
  {                                  // allowed combinations are:
    if (c=='A') i=random(0,12);      // AA-AL, or
    else i=random(0,26);             // KA-KZ, NA-NZ, WA-WZ 
    addChar(call,'A'+i);             // add second char to prefix                                                         
  }
  addChar(call,randomNumber());      // add zone number to callsign
  for (int i=0; i<random(1, 4); i++) // Suffix contains 1-3 letters
    addChar(call,randomLetter());    // add suffix letter(s) to call
}

void sendNumbers()                   // send random numbers forever...
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)   // break them up into "words"
      sendCharacter(randomNumber()); // send a number
    sendCharacter(' ');              // send a space between number groups
  }
}

void sendLetters()                   // send random letters forever...
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)   // break them up into "words"
      sendCharacter(randomLetter()); // send the letter 
    sendCharacter(' ');              // send a space between words
  }
}

void sendMixedChars()                // send letter/number groups...
{ 
  while (!button_pressed) {                            
    for (int i=0; i<WORDSIZE; i++)   // break them up into "words"
    {
      int c = '0' + random(43);      // pick a random character
      sendCharacter(c);              // and send it
    }
    sendCharacter(' ');              // send a space between words
  }
}

void sendHamWords()                  // send some common ham words
{ 
  while (!button_pressed) {
    int index=random(0, ELEMENTS(hamWords)); // eeny, meany, miney, moe
    sendString(hamWords[index]);     // send the word
    sendCharacter(' ');              // and a space between words
  }
}

void sendCommonWords()
{ 
  while (!button_pressed) {
    sendString(commonWords);         // one long string of 100 words
  }
}

void sendCallsigns()                 // send random US callsigns
{ 
  char call[8];                      // need string to stuff callsign into
  while (!button_pressed) {
    createCallsign(call);            // make it
    sendString(call);                // and send it
    sendCharacter(' ');
  }
}

bool ditPressed()
{
  return (digitalRead(PADDLE_A)==0); // pin is active low
}

bool dahPressed()
{
  return (digitalRead(PADDLE_B)==0); // pin is active low
}

void doPaddles()
{
  while (true) {
    if (ditPressed())  dit();        // user wants a dit, so do it.
    if (dahPressed())  dah();        // user wants a dah, so do it.
  }
}

void clearScreen() {
  tft.fillScreen(BLACK);             // yes, so clear screen
  textRow=0; textCol=0;              // and start on first row  
}

void showCharacter(char c, int row, int col)  // display a character at given row & column
{
  int x = col * COLSPACING;          // convert column to x coordinate
  int y = TOPDEADSPACE + (row * ROWSPACING);  // convert row to y coordinate
  tft.setCursor(x,y);                // position character on screen
  tft.print(c);                      // and display it 
}

void addCharacter(char c)
{
  showCharacter(c,textRow,textCol);  // display character & current row/col position
  textCol++;                         // go to next position on the current row
  if ((textCol>=MAXCOL) ||           // are we at end of the row?
     ((c==' ') && (textCol>MAXCOL-7)))  // or at a wordspace thats near end of row?
  {
    textRow++; textCol=0;            // yes, so advance to beginning of next row
    if (textRow >= MAXROW)           // but have we run out of rows?
    {
      clearScreen();                 // start at top again
    }
  }
}

void addBunchOfCharacters()          // just for testing...
{
  for (int i=0; i<200; i++)
    addCharacter(randomLetter());
}

//===================================  Rotary Encoder Code  =============================
/* 
   Rotary Encoder Button Interrupt Service Routine ----------
   Process encoder button presses and releases, including debouncing (extra "presses" from 
   noisy switch contacts). If button is pressed, the button_pressed flag is set to true. If 
   button is released, the button_released flag is set to true, and button_downtime will 
   contain the duration of the button press in ms.  Manually reset flags after handling event. 
*/

void buttonISR()
{  
  static boolean button_state = false;
  static unsigned long start, end;  
  boolean pinState = digitalRead(ENCODER_BUTTON);
  
  if ((pinState==LOW) && (button_state==false))                     
  {                                  // Button was up, but is now down
    start = millis();                // mark time of button down
    if (start > (end + 10))          // was button up for 10mS?
    {
      button_state = true;           // yes, so change state
      button_pressed = true;
    }
  }
  else if ((pinState==HIGH) && (button_state==true))                       
  {                                  // Button was down, but now up
    end = millis();                  // mark time of release
    if (end > (start + 10))          // was button down for 10mS?
    {
      button_state = false;          // yes, so change state
      button_released = true;
      button_downtime = end - start; // and record how long button was down
    }
  }
}

/* 
   Rotary Encoder Interrupt Service Routine ---------------
   This function will runs when either encoder pin A or B changes
   state.  The states array maps each transition 0000..1111 into 
   CW/CCW rotation (or invalid).   The rotary "position" is held
   in rotary_counter, increasing for CW rotation, decreasing 
   for CCW rotation. If the position changes, rotary_change will
   be set true. 
   You should set this to false after handling the change.
   To implement, attachInterrupts to encoder pin A *and* pin B 
*/

 
void rotaryISR()
{
  const int states[] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};
  static byte rotary_state = 0;      // holds current and previous encoder states   

  rotary_state <<= 2;                // shift previous state up 2 bits
  rotary_state |= (digitalRead(ENCODER_A));       // put encoder_A on bit 0
  rotary_state |= (digitalRead(ENCODER_B) << 1);  // put encoder_B on bit 1
  rotary_state &= 0x0F;              // zero upper 4 bits

  int change = states[rotary_state]; // map transition to CW vs CCW rotation
  if (change!=0)                     // make sure transition is valid
  {
    rotary_changed = true;                          
    rotary_counter += change;        // update rotary counter +/- 1
  }
}

/* 
   readEncoder() returns 0 if no significant encoder movement 
   since last call,  +1 if clockwise rotation, and -1 for 
   counter-clockwise rotation
*/

int readEncoder(int numTicks = ENCODER_TICKS) 
{
  static int prevCounter = 0;                     // holds last encoder position
  rotary_changed = false;                         // Clear flag
  int change = rotary_counter - prevCounter;      // how many ticks since last call?
  if (abs(change)<=numTicks)                      // not enough ticks?
    return 0;                                     // so exit with a 0.
  prevCounter = rotary_counter;                   // enough clicks, so save current counter values
  return (change>0) ? 1:-1;                       // return +1 for CW rotation, -1 for CCW    
}

void initEncoder()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON),buttonISR,CHANGE);   
  attachInterrupt(digitalPinToInterrupt(ENCODER_A),rotaryISR,CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_B),rotaryISR,CHANGE); 
}

//===================================  Menu Routines ====================================

void eraseMenus()                    // clear the text portion of the display
{
  tft.fillRect(0, TOPDEADSPACE, DISPLAYWIDTH, DISPLAYHEIGHT, BLACK);
  tft.setTextColor(TEXTCOLOR,BLACK);
  tft.setCursor(0,TOPDEADSPACE);
}

int getMenuSelection()           // Display menu system & get user selection
{
  int item;
  eraseMenus();                      // start with fresh screen
  menuCol = topMenu(mainMenu,ELEMENTS(mainMenu)); // show horiz menu & get user choice
  switch (menuCol)                   // now show menu that user selected:
  {
   case 0: 
     item = subMenu(menu0,ELEMENTS(menu0));  // "receive" dropdown menu
     break;
   case 1: 
     item = subMenu(menu1,ELEMENTS(menu1));  // "send" dropdown menu
     break;
   case 2: 
     item = subMenu(menu2,ELEMENTS(menu2));  // "config" dropdown menu
  }
  return (menuCol*10 + item);                // return user's selection
}

void showMenuItem(char *item, int x, int y, int fgColor, int bgColor)
{
  tft.setCursor(x,y);
  tft.setTextColor(fgColor, bgColor);
  tft.print(item);  
}

int topMenu(char *menu[], int itemCount)     // Display a horiz menu & return user selection
{
  int index = menuCol;                       // start w/ current row
  tft.setTextSize(2);                        // sets menu text size
  button_pressed = false;                    // reset button flag

  for (int i = 0; i < itemCount; i++)        // for each item in menu                         
    showMenuItem(menu[i],i*MENUSPACING,0,FG,BG);// display it 

  showMenuItem(menu[index],index*MENUSPACING,   // highlight current item
    0,SELECTFG,SELECTBG);
  tft.drawLine(0,TOPDEADSPACE-4,DISPLAYWIDTH,
    TOPDEADSPACE-4, YELLOW);                    // horiz. line below menu

  while (!button_pressed)                       // loop for user input:
  {
    int dir = readEncoder();                    // check encoder
    if (dir) {                                  // did it move?
      showMenuItem(menu[index],index*MENUSPACING,
      0, FG,BG);                                // deselect current item
      index += dir;                             // go to next/prev item
      if (index > itemCount-1) index=0;         // dont go beyond last item
      if (index < 0) index = itemCount-1;       // dont go before first item
      showMenuItem(menu[index],index*MENUSPACING,
      0, SELECTFG,SELECTBG);                    // select new item  
    }  
  }
  return index;  
}

int subMenu(char *menu[], int itemCount) // Display drop-down menu & return user selection
{
  int index=0, x,y; 
  button_pressed = false;                // reset button flag

  x = menuCol * MENUSPACING;             // x-coordinate of this menu
  for (int i = 0; i < itemCount; i++)    // for all items in the menu...
  {
     y = TOPDEADSPACE + i*ROWSPACING;    // calculate y coordinate
     showMenuItem(menu[i],x,y,FG,BG);    // and show the item.
  }
  showMenuItem(menu[index],x,TOPDEADSPACE,  // highlight selected item
    SELECTFG,SELECTBG);

  while (!button_pressed)            // exit on button press
  {
    int dir = readEncoder();         // check for encoder movement
    if (dir)                         // it moved!    
    {
      y = TOPDEADSPACE + index*ROWSPACING; // calc y-coord of current item
      showMenuItem(menu[index],x,y,FG,BG); // deselect current item
      index += dir;                        // go to next/prev item
      if (index > itemCount-1) index=0;    // dont go past last item
      if (index < 0) index = itemCount-1;  // dont go before first item
       y = TOPDEADSPACE + index*ROWSPACING;   // calc y-coord of new item
      showMenuItem(menu[index],x,y,
      SELECTFG,SELECTBG);                  // select new item
    }
  }
  return index;  
}


void setup() {
  initEncoder();
  pinMode(LED,OUTPUT);
  pinMode(PADDLE_A, INPUT_PULLUP);     // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);
  tft.begin();                         // initialize screen object
  tft.setRotation(1);                  // landscape mode
  tft.fillScreen(BLACK);               // start with blank screen
}

void loop()
{
  int selection = getMenuSelection();  // get menu selection from user
  eraseMenus(); textRow=0; textCol=0;  // clear screen below menu
  button_pressed = false;              // reset flag for new presses
  randomSeed(millis());                // randomize!
  switch(selection)                    // do action requested by user
  {
    case 00: sendLetters(); break;
    case 01: sendNumbers(); break;
    case 03: sendCommonWords(); break;
    case 04: sendMixedChars(); break;
    case 05: sendCallsigns(); break;
    default: ;
  }
}
