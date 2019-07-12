/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   26 Jun 2019
    Hardware:   STM32F103C "Blue Pill" with Piezo Buzzer
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
    
 Description:   Morse Code Demo, using LED & Buzzer outputs.  
                Builds on SimpleMorse, adding routines for sending
                random letters, numbers, words, and callsigns.
   
 **************************************************************************/
/**************************************************************************
Modification:   'port' to Mega2560 by Ken, KM4NFQ "Not Fully Qualified"
        Date:   5 July 2019
    Hardware:   RobotDyn Mega2560 (no USB port) with buzzer and paddles
    Software:   Arduino IDE 1.8.9 running on Debian GNU/Linux system
 Description:   Part 3 of Morse Tutor tutorial, using a Mega2560 Pro Mini.
                Adding a paddle to the circuit.
                If a speaker is added to the circuit, make sure that it
                does not exceed the absolute maximum current from pin 26.
                You can put a resistor in series with a 4 or 8 Ohm speaker 
                to make it "safe", but the volume will be reduced. If it's 
                loud enough with a resistor, fine.  Otherwise, use an audio 
                amplifier or "powered" computer speakers. All lines in the 
                code with the letters 'CHG' in the comments were modified.
 **************************************************************************/

#define CODESPEED   13                       // speed in Words per Minute
#define PITCH       700                      // CHG pitch in Hz of morse audio
#define LED         13                       // CHG onboard LED pin
#define PIEZO       26                       // CHG pin attached to piezo element
#define DITPERIOD   1200/CODESPEED           // period of dit, in milliseconds
#define WORDSIZE    5                        // number of chars per random word
#define PADDLE_A    33                       // CHG Morse Paddle "dit"
#define PADDLE_B    35                       // CHG Morse Paddle "dah"
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

char *greetings   = "Greetings de KM4NFQ = Name is Ken = +"; // CHG
char *commonWords = "the of and to a in that is was he for it with as his on be at by I this had not are but from or have an they which one you were all her she there would their we him been has when who will no more if out so up said what its about than into them can only other time new some could these two may first then do any like my now over such our man me even most made after also did many off before must well back through years much where your way"; // CHG
char *hamWords[]  = {"DE", "TNX FER", "BT", "WX", "HR", "TEMP", "ES", "RIG", "ANT", "DIPOLE", "VERTICAL", // 0-10
                    "BEAM", "HW", "CPI", "WARM", "SUNNY", "CLOUDY", "COLD", "RAIN", "SNOW", "FOG",       // 11-20
                    "RPT", "NAME", "QTH", "CALL", "UR", "SLD", "FB", "RST"                               // 21-28
                    };
char prefix[]     = {'A', 'W', 'K', 'N'};

const byte morse[] = {                       // Each character is encoded into an 8-bit byte:
  0b01001010,        // ! exclamation        // Some punctuation not commonly used
  0b01101101,        // " quotation          // and taken for wikipedia: International Morse Code
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
  0b00100000,        // 0                    // Read the bits from RIGHT to left,   
  0b00100001,        // 1                    // with a "1"=dit and "0"=dah
  0b00100011,        // 2                    // example: 2 = 11000 or dit-dit-dah-dah-dah
  0b00100111,        // 3                    // the final bit is always 1 = stop bit.
  0b00101111,        // 4                    // see "sendElements" routine for more info.
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

void dit() {
  digitalWrite(LED,1);                       // CHG turn on LED
  tone(PIEZO,PITCH);                         // and turn on sound
  ditSpaces();                               // wait for period of 1 dit
  digitalWrite(LED,0);                       // CHG urn off LED
  noTone(PIEZO);                             // and turn off sound
  ditSpaces();                               // space between code elements
}

void dah() {
  digitalWrite(LED,1);                       // CHG turn on LED
  tone(PIEZO,PITCH);                         // and turn on sound
  ditSpaces(3);                              // length of dah = 3 dits
  digitalWrite(LED,0);                       // CHG turn off LED
  noTone(PIEZO);                             // and turn off sound
  ditSpaces();                               // space between code elements
}

// the following routine accepts a numberic input, where each bit represents a code element:
// 1=dit and 0=dah.   The elements are read right-to-left.  The left-most bit is a stop bit.
// For example:  5 = binary 0b0101.  The right-most bit is 1 so the first element is a dit.
// The next element to the left is a 0, so a dah follows.  Only a 1 remains after that, so
// the character is complete: dit-dah = morse character 'A'.

void sendElements(int x) {                   // send string of bits as Morse      
  while (x>1) {                              // stop when value is 1 (or less)
    if (x & 1) dit();                        // right-most bit is 1, so dit
    else dah();                              // right-most bit is 0, so dah
    x >>= 1;                                 // rotate bits right
  }
  ditSpaces(2);                              // add inter-character spacing
}

void sendCharacter(char c) {                 // send a single ASCII character in Morse
  if (c<32) return;                          // ignore control characters
  if (c>96) c -= 32;                         // convert lower case to upper case
  if (c>90) return;                          // not a character
  if (c==32) ditSpaces(5);                   // space between words 
  else sendElements(morse[c-33]);            // send the character
}

void sendString (char *ptr) {             
  while (*ptr)                               // send the entire string
    sendCharacter(*ptr++);                   // one character at a time
}

void addChar (char* str, char ch)            // adds 1 character to end of string
{                                            
  char c[2] = " ";                           // happy hacking: char into string
  c[0] = ch;                                 // change char'A' to string"A"
  strcat(str,c);                             // and add it to end of string
}

char randomLetter()                          // returns a random uppercase letter
{
  return 'A'+random(0,26);
}

char randomNumber()                          // returns a random single-digit # 0-9
{
  return '0'+random(0,10);
}


void createCallsign(char* call)              // returns with random US callsign in "call"
{  
  strcpy(call,"");                           // start with empty callsign                      
  int i = random(0, 4);                      // 4 possible start letters for US                                       
  char c = prefix[i];                        // Get first letter of prefix
  addChar(call,c);                           // and add it to callsign
  i = random(0,3);                           // single or double-letter prefix?
  if ((i==2) or (c=='A'))                    // do a double-letter prefix
  {                                          // allowed combinations are:
    if (c=='A') i=random(0,12);              // AA-AL, or
    else i=random(0,26);                     // KA-KZ, NA-NZ, WA-WZ 
    addChar(call,'A'+i);                     // add second char to prefix                                                         
  }
  addChar(call,randomNumber());              // add zone number to callsign
  for (int i=0; i<random(1, 4); i++)         // Suffix contains 1-3 letters
    addChar(call,randomLetter());            // add suffix letter(s) to call
}

void sendNumbers()                           // send random numbers forever...
{ 
  while (true) {
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
      sendCharacter(randomNumber());         // send a number
    sendCharacter(' ');                      // send a space between number groups
  }
}

void sendLetters()                           // send random letters forever...
{ 
  while (true) {
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
      sendCharacter(randomLetter());         // send the letter 
    sendCharacter(' ');                      // send a space between words
  }
}

void sendMixedChars()                        // send letter/number groups...
{ 
  while (true) {                            
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
    {
      int c = '0' + random(43);              // pick a random character
      sendCharacter(c);                      // and send it
    }
    sendCharacter(' ');                      // send a space between words
  }
}

void sendHamWords()                          // send some common ham words
{ 
  while (true) {
    int index=random(0, ELEMENTS(hamWords)); // eeny, meany, miney, moe
    sendString(hamWords[index]);             // send the word
    sendCharacter(' ');                      // and a space between words
  }
}

void sendCommonWords()
{ 
  while (true) {
    sendString(commonWords);                 // one long string of 100 words
  }
}

void sendCallsigns()                         // send random US callsigns
{ 
  char call[8];                              // need string to stuff callsign into
  while (true) {
    createCallsign(call);                    // make it
    sendString(call);                        // and send it
    sendCharacter(' ');
  }
}

bool ditPressed()
{
  return (digitalRead(PADDLE_A)==0);             // pin is active low
}

bool dahPressed()
{
  return (digitalRead(PADDLE_B)==0);             // pin is active low
}

void doPaddles()
{
  while (true) {
    if (ditPressed())  dit();                   // user wants a dit, so do it.
    if (dahPressed())  dah();                   // user wants a dah, so do it.
  }
}

void setup() {
  pinMode(LED,OUTPUT);
  pinMode(PADDLE_A, INPUT_PULLUP);            // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);            
 
  //uncomment any of the routines below to try them out...
  
  //sendWords();                               
  //sendLetters();
  //sendNumbers();
  //sendMixedChars();
  //sendCallsigns();
  //sendCommonWords();
  doPaddles();                               // CHG
}

void loop() {
  sendString(greetings);                      // just in case you didn't try anything above.
  delay(5000);
}
