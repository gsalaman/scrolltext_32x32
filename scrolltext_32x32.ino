// tweaked scrolltext demo for Adafruit RGBmatrixPanel library.
// Added clock, removed bouncy-balls.

#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

// Similar to F(), but for PROGMEM string pointers rather than literals
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

#define CLK 11  // MUST be on PORTB! (Use pin 11 on Mega)
#define LAT 10
#define OE  9
#define A   A0
#define B   A1
#define C   A2
#define D   A3
// Last parameter = 'true' enables double-buffering, for flicker-free,
// buttery smooth animation.  Note that NOTHING WILL SHOW ON THE DISPLAY
// until the first call to swapBuffers().  This is normal.
RGBmatrixPanel matrix(A, B, C,  D,  CLK, LAT, OE, false);
// Double-buffered mode consumes nearly all the RAM available on the
// Arduino Uno -- only a handful of free bytes remain.  Even the
// following string needs to go in PROGMEM:

const char str[] PROGMEM = "This space for rent";
int    textX   = matrix.width(),
       textMin = sizeof(str) * -6,
       hue     = 0;

// clock variables.
int hour=12;
int minute=0;
int second=0;
unsigned long last_update=0;

typedef enum
{
  MODE_NORMAL,
  MODE_ENTER_TIME_SET,
  MODE_SET_HOUR,
  MODE_WAIT_HOUR_RELEASE,
  MODE_SET_MINUTE,
  MODE_WAIT_NORMAL_RELEASE
} mode_type;

mode_type current_mode=MODE_NORMAL;

// We'll use pin A5 as our button pin.
#define BUTTON_PIN A5
int button_state;

#define BUTTON_HOLD_TIME 1000
unsigned long button_press_start=0;

void setup() {
  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge
  matrix.setTextSize(1);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  button_state = HIGH;
  
  // draw the "zeroth" pixel for the second hand.  Only needed on boot.
  matrix.drawPixel(1,20,matrix.Color333(0,1,0));
  display_time();
}

void display_time( void )
{

   matrix.setTextColor(matrix.Color333(1,0,0));
  
   // time goes from 1,12 to 32,20
   matrix.fillRect(1,12,32,8,0);

   if (hour > 9)
   {
     matrix.setCursor(1,12);
   }
   else
   {
     matrix.setCursor(7,12);
   }
  
   matrix.print(hour);
   matrix.print(':');
   
   if (minute < 10) matrix.print('0');
   matrix.print(minute);

   #if 0
   matrix.print(':');
   if (second < 10) matrix.print('0');
   matrix.print(second);
   #endif
}

void tick_hour( void )
{
  hour++;
  if (hour > 12) hour = 1;
}

void tick_minute( void )
{
  minute++;
  if (minute > 59) 
  {
    minute = 0;
    tick_hour();
  }

  display_time();
  
}

void inc_minute( void )
{
  minute++;
  if (minute > 59) minute = 0;
}

void tick_second( void )
{
  int x;
  int y;
  
  second++;
  if (second > 59) 
  {
    second = 0;
    tick_minute();

    // Clear our seconds bar
    matrix.drawRect(0,20,32,2,0);

    // amd draw the "zeroth" pixel
    matrix.drawPixel(1,20,matrix.Color333(0,1,0));
  }
  else
  {
    // set the next pixel in the seconds bar
    y = 20+second % 2;
    x = 1 + (second / 2);
    
    matrix.drawPixel(x,y,matrix.Color333(0,1,0));
    
  }

}

void update_clock( void )
{
  unsigned long curr_time;

    
  curr_time = millis();
  
  if (curr_time >= last_update + 1000)
  {
    tick_second();
    last_update = last_update + 1000;
    
    //display_time();
  }

    
}

void show_message( void )
{

  // Clear background...but probalby just want to blank out the top.
  //matrix.fillScreen(0);
  matrix.fillRect(0,0,32,10,0);
  
  update_clock();
  
  // Draw big scrolly text on top
  matrix.setTextColor(matrix.Color333(1,0,0));
  matrix.setCursor(textX, 1);
  matrix.print(F2(str));

  // Move text left (w/wrap)
  if((--textX) < textMin) textX = matrix.width();

  // Update display
  //matrix.swapBuffers(true);

  delay(50);
  
}

bool check_button_hold( void )
{
  int current_button_state;
  unsigned long current_time;

  current_time = millis();
  current_button_state = digitalRead(BUTTON_PIN);
  
  // if we just started pressing the button, mark the time.
  if ((button_state == HIGH) && (current_button_state == LOW))
  {
    button_press_start = current_time;
    button_state = LOW;
  }

  // if the button has been down long enough, mark it as a hold.
  if ((current_button_state == LOW) && 
      (button_state == LOW) && 
      (current_time > button_press_start + BUTTON_HOLD_TIME))
  {
    return true;
  }
  else
  {
    return false;
  }
  
}

void set_hour( void )
{
  int current_button_state;
  unsigned long current_time;

    
  // on Entry to this state, the button should have just been released (HIGH).
  // Two things to look for:
  // If we detect a button "hold" event, we want to go on to setting the minute.
  // if we detect a quick press, we want to increment the hour.

  current_button_state = digitalRead(BUTTON_PIN);
  current_time = millis();
  
  if ( (button_state == LOW) && (current_button_state == HIGH))
  {
    // That's a release.  Increment hour.
    tick_hour();
    display_time();
  }
  else if ((button_state == HIGH) && (current_button_state == LOW))
  {
    // That's the start of a press.  Mark time the button went down.
    button_press_start = current_time;
  }
  else if ((button_state == LOW) && 
           (current_button_state == LOW) &&
           (current_time > button_press_start + BUTTON_HOLD_TIME))
  {
    // That's a hold.  Go to tweaking minutes.
    current_mode = MODE_WAIT_HOUR_RELEASE;
            
    // let user know we're waiting for another release
    matrix.fillRect(0,0,32,10,0);
    matrix.setCursor(0,0);
    matrix.print("LetGo");
    
  }

  button_state = current_button_state;
}

void set_minute( void )
{
  int current_button_state;
  unsigned long current_time;

    
  // on Entry to this state, the button should have just been released (HIGH).
  // Two things to look for:
  // If we detect a button "hold" event, we want to go back to normal mode.
  // if we detect a quick press, we want to increment the minute.

  current_button_state = digitalRead(BUTTON_PIN);
  current_time = millis();
  
  if ( (button_state == LOW) && (current_button_state == HIGH))
  {
    // That's a release.  Increment minute.
    inc_minute();
    display_time();
  }
  else if ((button_state == HIGH) && (current_button_state == LOW))
  {
    // That's the start of a press.  Mark time the button went down.
    button_press_start = current_time;
  }
  else if ((button_state == LOW) && 
           (current_button_state == LOW) &&
           (current_time > button_press_start + BUTTON_HOLD_TIME))
  {
    // That's a hold.  
    current_mode = MODE_WAIT_NORMAL_RELEASE;
        
    // let user know we're waiting for another release
    matrix.fillRect(0,0,32,10,0);
    matrix.setCursor(0,0);
    matrix.print("LetGo");
  }

  button_state = current_button_state;
}
void loop() 
{
  switch (current_mode)
  {
    case MODE_NORMAL:
      show_message();
      if (check_button_hold())
      {
        // let user know we're setting time
        matrix.fillRect(0,0,32,10,0);
        matrix.setCursor(0,0);
        matrix.print("LetGo");    
        
        current_mode = MODE_ENTER_TIME_SET;
      }      
    break;

    case MODE_ENTER_TIME_SET:
      // wait for button release
      if (digitalRead(BUTTON_PIN) == HIGH)
      {
         matrix.fillRect(0,0,32,10,0);
         matrix.setCursor(0,0);
         matrix.print("Hour");

         button_state = HIGH;

         current_mode = MODE_SET_HOUR;
      }
    break;

    case MODE_SET_HOUR:
      set_hour();
    break;

    case MODE_WAIT_HOUR_RELEASE:
      if (digitalRead(BUTTON_PIN) == HIGH)
      {
         matrix.fillRect(0,0,32,10,0);
         matrix.setCursor(0,0);
         matrix.print("Min");

         button_state = HIGH;
         current_mode = MODE_SET_MINUTE;
      }
    break;

    case MODE_SET_MINUTE:
      set_minute();
    break;  

    case MODE_WAIT_NORMAL_RELEASE:
      if (digitalRead(BUTTON_PIN) == HIGH)
      {
         second=0;
         last_update = millis();
         button_state = HIGH;
         current_mode = MODE_NORMAL;
             
         // Clear our seconds bar
         matrix.drawRect(0,20,32,2,0);

         // amd draw the "zeroth" pixel
         matrix.drawPixel(1,20,matrix.Color333(0,1,0));
      }
    break;
    
  }
}
