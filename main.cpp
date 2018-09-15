#include "mbed.h"
#include "TextLCD.h"

// Initialize objects for LCD and threads
// TextLCD config taken from CIS541 slides
TextLCD lcd(p15,p16,p17,p18,p19,p20, TextLCD::LCD16x2);
Serial pc(USBTX, USBRX);

Thread ms_thread;
Thread sec_thread;
Thread minute_thread;
Thread lcd_thread;
Thread keypad_thread;

// Event flags for the second and minute threads
EventFlags second_inc;
EventFlags minute_inc;
EventFlags update_lcd;  // Also have one to refresh the LCD

// Need a ticker for the ms thread
Ticker ms_tick;

// Shared count variables
unsigned int ms_count;
unsigned int sec_count;
unsigned int minute_count;

// Inidicates if the LCD is paused
bool pauseFlag;

// Indicates if the timer has been reset or the program just initialized
bool resetFlag;

void init();
void ms_increment();
void sec_increment();
void minute_increment();

// Refreshes the LCD output to display the most up-to-date time
void lcd_refresh()
{
    while(1) {
        
        // Wait for the LCD flag
        update_lcd.wait_all(0x1);
        
        // Move the cursor to 0,0 so formatting is correct
        lcd.locate(0,0);
        
        // Print the current time
        lcd.printf("%02d:%02d:%03d",minute_count,sec_count,ms_count);
    }
}



// Increments the millisecond timer when the ticker counts a ms
void ms_increment()
{
    // When you reach 1000 ms, need to rollover and call the seconds thread
    if (ms_count < 999) {
        ms_count++;
    } else {
        ms_count = 0;
        second_inc.set(0x1);
    }

    //If the LCD isn't paused, refresh it
    if(!pauseFlag) {
        update_lcd.set(0x1);
    }
}

// Waits for a signal from the millisecond thread
// and increment the seconds counter
void sec_increment()
{
    while (1) {
        second_inc.wait_all(0x1);

        // If sec_count is 59 need to rollover and call the minute thread
        if (sec_count < 59) {
            sec_count ++;
        } else {
            sec_count = 0;
            minute_inc.set(0x1);
        }
    }
}

// Waits for a signal from the seconds thread
// and then incrememnts the minutes counter
void minute_increment()
{
    while (1) {
        minute_inc.wait_all(0x1);

        // Only have 2 digits, so this has to roll over and start everything
        // from 00:00:000 when the timer hits 99:99:999
        if (minute_count < 99) {
            minute_count ++;
        } else {
            minute_count = 0;
        }
    }
}

// This thread reads a keystroke from the user and decides if a state change
// is needed for the timer. Valid keystrokes are exclusively 's','r',and 'p'.
void read_keypad()
{

    
    DigitalOut myled(LED1);
    while (1) {
        //update_lcd.set(0x1);
        
        //Get keystroke
        char c = pc.getc();
        
        // Pause the LCD if it was running
        if((c == 'p') && !pauseFlag) {
            pauseFlag = true;
            //  update_lcd.wait_all(0x1);
            myled = 1;
            
        // Resume the timer if it was paused
        } else if (c == 'p' && pauseFlag) {
            // update_lcd.set(0x1);
            pauseFlag = false;
            myled = 0;
            
        // Start the timer, assuming it was not previously running
        } else if(c == 's' && resetFlag) {
            
            // Initialize count values and make sure LCD is not paused
            ms_count = 0;
            sec_count = 0;
            minute_count = 0;
            pauseFlag = false;
            
            // Starting the timer is essentially taking it out of reset mode
            resetFlag = false;
            
            // Increment the milliseconds timer every 0.001 second, or 1 ms
            ms_tick.attach(ms_increment, 0.001);
            
        // Reset the timer
        } else if(c == 'r' && pauseFlag) {
            ms_tick.detach();
            init();
        }
        //lcd.locate(0,0);
    }
}

// Initialize the timer variables, set the LCD to 00:00:000,
// and start all the threads
void init()
{
    ms_count = 0;
    sec_count = 0;
    minute_count = 0;
    pauseFlag = true;
    resetFlag = true;
    lcd.locate(0,0);
    lcd.printf("%02d:%02d:%03d\n",minute_count,sec_count,ms_count);

    // Start threads
    lcd_thread.start(lcd_refresh);
    keypad_thread.start(read_keypad);
    sec_thread.start(sec_increment);
    minute_thread.start(minute_increment);
}

// Main essentially just sets up and starts all other threads by calling
// init(), then is simply the thread where the ms ticker resides
int main()
{
    init();
    while (1) {

    }
}


