#ifndef ULTRA_LCD_IMPLEMENTATION_HITACHI_HD44780_H
#define ULTRA_LCD_IMPLEMENTATION_HITACHI_HD44780_H

#ifdef TENLOG_CONTROLLER
#include "Marlin.h"
void TenlogScreen_println(String s);
void TenlogScreen_print(String s);
void TenlogScreen_printend();
#endif

//by zyf
#ifdef SHOW_BOOTSCREEN_2004
static void bootscreen();
static bool show_bootscreen = true;
#endif

/**
* Implementation of the LCD display routines for a hitachi HD44780 display. These are common LCD character displays.
* When selecting the rusian language, a slightly different LCD implementation is used to handle UTF8 characters.
**/

#ifndef REPRAPWORLD_KEYPAD
extern volatile uint8_t buttons;  //the last checked buttons in a bit array.
#else
extern volatile uint16_t buttons;  //an extended version of the last checked buttons in a bit array.
#endif

////////////////////////////////////
// Setup button and encode mappings for each panel (into 'buttons' variable
//
// This is just to map common functions (across different panels) onto the same 
// macro name. The mapping is independent of whether the button is directly connected or 
// via a shift/i2c register.

#ifdef ULTIPANEL
    // All Ultipanels might have an encoder - so this is always be mapped onto first two bits
    #define BLEN_B 1
    #define BLEN_A 0

    #define EN_B (1<<BLEN_B) // The two encoder pins are connected through BTN_EN1 and BTN_EN2
    #define EN_A (1<<BLEN_A)

    #if defined(BTN_ENC) && BTN_ENC > -1
        // encoder click is directly connected
        #define BLEN_C 2 
        #define EN_C (1<<BLEN_C) 
    #endif 

    //
    // Setup other button mappings of each panel
    //
    #if defined(LCD_I2C_VIKI)
        #define B_I2C_BTN_OFFSET 3 // (the first three bit positions reserved for EN_A, EN_B, EN_C)

        // button and encoder bit positions within 'buttons'
        #define B_LE (BUTTON_LEFT<<B_I2C_BTN_OFFSET)    // The remaining normalized buttons are all read via I2C
        #define B_UP (BUTTON_UP<<B_I2C_BTN_OFFSET)
        #define B_MI (BUTTON_SELECT<<B_I2C_BTN_OFFSET)
        #define B_DW (BUTTON_DOWN<<B_I2C_BTN_OFFSET)
        #define B_RI (BUTTON_RIGHT<<B_I2C_BTN_OFFSET)

        #if defined(BTN_ENC) && BTN_ENC > -1 
            // the pause/stop/restart button is connected to BTN_ENC when used
            #define B_ST (EN_C)                            // Map the pause/stop/resume button into its normalized functional name 
            #define LCD_CLICKED (buttons&(B_MI|B_RI|B_ST)) // pause/stop button also acts as click until we implement proper pause/stop.
        #else
            #define LCD_CLICKED (buttons&(B_MI|B_RI))
        #endif  

        // I2C buttons take too long to read inside an interrupt context and so we read them during lcd_update
        #define LCD_HAS_SLOW_BUTTONS

    #elif defined(LCD_I2C_PANELOLU2)
        // encoder click can be read through I2C if not directly connected
        #if BTN_ENC <= 0 
            #define B_I2C_BTN_OFFSET 3 // (the first three bit positions reserved for EN_A, EN_B, EN_C)
            #define B_MI (PANELOLU2_ENCODER_C<<B_I2C_BTN_OFFSET) // requires LiquidTWI2 library v1.2.3 or later
            #define LCD_CLICKED (buttons&B_MI)
        // I2C buttons take too long to read inside an interrupt context and so we read them during lcd_update
            #define LCD_HAS_SLOW_BUTTONS
        #else
            #define LCD_CLICKED (buttons&EN_C)  
        #endif

    #elif defined(REPRAPWORLD_KEYPAD)
        // define register bit values, don't change it
        #define BLEN_REPRAPWORLD_KEYPAD_F3 0
        #define BLEN_REPRAPWORLD_KEYPAD_F2 1
        #define BLEN_REPRAPWORLD_KEYPAD_F1 2
        #define BLEN_REPRAPWORLD_KEYPAD_UP 3
        #define BLEN_REPRAPWORLD_KEYPAD_RIGHT 4
        #define BLEN_REPRAPWORLD_KEYPAD_MIDDLE 5
        #define BLEN_REPRAPWORLD_KEYPAD_DOWN 6
        #define BLEN_REPRAPWORLD_KEYPAD_LEFT 7

        #define REPRAPWORLD_BTN_OFFSET 3 // bit offset into buttons for shift register values

        #define EN_REPRAPWORLD_KEYPAD_F3 (1<<(BLEN_REPRAPWORLD_KEYPAD_F3+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_F2 (1<<(BLEN_REPRAPWORLD_KEYPAD_F2+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_F1 (1<<(BLEN_REPRAPWORLD_KEYPAD_F1+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_UP (1<<(BLEN_REPRAPWORLD_KEYPAD_UP+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_RIGHT (1<<(BLEN_REPRAPWORLD_KEYPAD_RIGHT+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_MIDDLE (1<<(BLEN_REPRAPWORLD_KEYPAD_MIDDLE+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_DOWN (1<<(BLEN_REPRAPWORLD_KEYPAD_DOWN+REPRAPWORLD_BTN_OFFSET))
        #define EN_REPRAPWORLD_KEYPAD_LEFT (1<<(BLEN_REPRAPWORLD_KEYPAD_LEFT+REPRAPWORLD_BTN_OFFSET))

        #define LCD_CLICKED ((buttons&EN_C) || (buttons&EN_REPRAPWORLD_KEYPAD_F1))
        #define REPRAPWORLD_KEYPAD_MOVE_Y_DOWN (buttons&EN_REPRAPWORLD_KEYPAD_DOWN)
        #define REPRAPWORLD_KEYPAD_MOVE_Y_UP (buttons&EN_REPRAPWORLD_KEYPAD_UP)
        #define REPRAPWORLD_KEYPAD_MOVE_HOME (buttons&EN_REPRAPWORLD_KEYPAD_MIDDLE)

    #endif

    ////////////////////////
    // Setup Rotary Encoder Bit Values (for two pin encoders to indicate movement)
    // These values are independent of which pins are used for EN_A and EN_B indications
    // The rotary encoder part is also independent to the chipset used for the LCD
    #if defined(EN_A) && defined(EN_B)
        #ifndef ULTIMAKERCONTROLLER
            #define encrot0 0
            #define encrot1 2
            #define encrot2 3
            #define encrot3 1
        #else
            #define encrot0 0
            #define encrot1 1
            #define encrot2 3
            #define encrot3 2
        #endif
    #endif 

#endif //ULTIPANEL

////////////////////////////////////
// Create LCD class instance and chipset-specific information
#if defined(LCD_I2C_TYPE_PCF8575)
// note: these are register mapped pins on the PCF8575 controller not Arduino pins
#define LCD_I2C_PIN_BL  3
#define LCD_I2C_PIN_EN  2
#define LCD_I2C_PIN_RW  1
#define LCD_I2C_PIN_RS  0
#define LCD_I2C_PIN_D4  4
#define LCD_I2C_PIN_D5  5
#define LCD_I2C_PIN_D6  6
#define LCD_I2C_PIN_D7  7

#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#define LCD_CLASS LiquidCrystal_I2C
LCD_CLASS lcd(LCD_I2C_ADDRESS,LCD_I2C_PIN_EN,LCD_I2C_PIN_RW,LCD_I2C_PIN_RS,LCD_I2C_PIN_D4,LCD_I2C_PIN_D5,LCD_I2C_PIN_D6,LCD_I2C_PIN_D7);

#elif defined(LCD_I2C_TYPE_MCP23017)
//for the LED indicators (which maybe mapped to different things in lcd_implementation_update_indicators())
#define LED_A 0x04 //100
#define LED_B 0x02 //010
#define LED_C 0x01 //001

#define LCD_HAS_STATUS_INDICATORS

#include <Wire.h>
#include <LiquidTWI2.h>
#define LCD_CLASS LiquidTWI2
LCD_CLASS lcd(LCD_I2C_ADDRESS);

#elif defined(LCD_I2C_TYPE_MCP23008)
#include <Wire.h>
#include <LiquidTWI2.h>
#define LCD_CLASS LiquidTWI2
LCD_CLASS lcd(LCD_I2C_ADDRESS);  

#elif defined(LCD_I2C_TYPE_PCA8574)
#include <LiquidCrystal_I2C.h>
#define LCD_CLASS LiquidCrystal_I2C
LCD_CLASS lcd(LCD_I2C_ADDRESS, LCD_WIDTH, LCD_HEIGHT);

#else
// Standard directly connected LCD implementations
#if LANGUAGE_CHOICE == 6
#include "LiquidCrystalRus.h"
#define LCD_CLASS LiquidCrystalRus
#else 
#include <LiquidCrystal.h>
#define LCD_CLASS LiquidCrystal
#endif  
LCD_CLASS lcd(LCD_PINS_RS, LCD_PINS_ENABLE, LCD_PINS_D4, LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7);  //RS,Enable,D4,D5,D6,D7
#endif

/* Custom characters defined in the first 8 characters of the LCD */
#define LCD_STR_BEDTEMP     "\x00"
#define LCD_STR_DEGREE      "\x01"
#define LCD_STR_THERMOMETER "\x02"
#define LCD_STR_UPLEVEL     "\x03"
#define LCD_STR_REFRESH     "\x04"
#define LCD_STR_FOLDER      "\x05"
#define LCD_STR_FEEDRATE    "\x06"
#define LCD_STR_CLOCK       "\x07"

#define LCD_STR_ARROW_RIGHT "\x7E"  /* from the default character set */

static void lcd_implementation_init()
{
    byte bedTemp[8] =
    {
        B00000,
        B11111,
        B10101,
        B10001,
        B10101,
        B11111,
        B00000,
        B00000
    }; //thanks Sonny Mounicou
    byte degree[8] =
    {
        B01100,
        B10010,
        B10010,
        B01100,
        B00000,
        B00000,
        B00000,
        B00000
    };
    byte thermometer[8] =
    {
        B00100,
        B01010,
        B01010,
        B01010,
        B01010,
        B10001,
        B10001,
        B01110
    };
    byte uplevel[8]={
        B00100,
        B01110,
        B11111,
        B00100,
        B11100,
        B00000,
        B00000,
        B00000
    }; //thanks joris
    byte refresh[8]={
        B00000,
        B00110,
        B11001,
        B11000,
        B00011,
        B10011,
        B01100,
        B00000,
    }; //thanks joris
    byte folder [8]={
        B00000,
        B11100,
        B11111,
        B10001,
        B10001,
        B11111,
        B00000,
        B00000
    }; //thanks joris
    byte feedrate [8]={
        B11100,
        B10000,
        B11000,
        B10111,
        B00101,
        B00110,
        B00101,
        B00000
    }; //thanks Sonny Mounicou
    byte clock [8]={
        B00000,
        B01110,
        B10011,
        B10101,
        B10001,
        B01110,
        B00000,
        B00000
    }; //thanks Sonny Mounicou

#if defined(LCDI2C_TYPE_PCF8575)
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#ifdef LCD_I2C_PIN_BL
    lcd.setBacklightPin(LCD_I2C_PIN_BL,POSITIVE);
    lcd.setBacklight(HIGH);
#endif

#elif defined(LCD_I2C_TYPE_MCP23017)
    lcd.setMCPType(LTI_TYPE_MCP23017);
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    lcd.setBacklight(0); //set all the LEDs off to begin with

#elif defined(LCD_I2C_TYPE_MCP23008)
    lcd.setMCPType(LTI_TYPE_MCP23008);
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);

#elif defined(LCD_I2C_TYPE_PCA8574)
    lcd.init();
    lcd.backlight();    
#else
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#endif

    //By Zyf
#ifdef  SHOW_BOOTSCREEN_2004
    if(show_bootscreen){
        bootscreen();
        show_bootscreen = false;
        card.initsd();
    }
#endif
    lcd.createChar(LCD_STR_BEDTEMP[0], bedTemp);
    lcd.createChar(LCD_STR_DEGREE[0], degree);
    lcd.createChar(LCD_STR_THERMOMETER[0], thermometer);    
    lcd.createChar(LCD_STR_UPLEVEL[0], uplevel);
    lcd.createChar(LCD_STR_REFRESH[0], refresh);
    lcd.createChar(LCD_STR_FOLDER[0], folder);
    lcd.createChar(LCD_STR_FEEDRATE[0], feedrate);
    lcd.createChar(LCD_STR_CLOCK[0], clock);
    lcd.clear();
}
static void lcd_implementation_clear()
{
    lcd.clear();
}
/* Arduino < 1.0.0 is missing a function to print PROGMEM strings, so we need to implement our own */
static void lcd_printPGM(const char* str)
{
    char c;
    while((c = pgm_read_byte(str++)) != '\0')
    {
        lcd.write(c);
    }
}
/*
Possible status screens:
16x2   |0123456789012345|
|000/000 B000/000|
|Status line.....|

16x4   |0123456789012345|
|000/000 B000/000|
|SD100%    Z000.0|
|F100%     T--:--|
|Status line.....|

20x2   |01234567890123456789|
|T000/000D B000/000D |
|Status line.........|

20x4   |01234567890123456789|
|T000/000D B000/000D |
|X+000.0 Y+000.0 Z+000.0|
|F100%  SD100% T--:--|
|Status line.........|

20x4   |01234567890123456789|
|T000/000D B000/000D |
|T000/000D     Z000.0|
|F100%  SD100% T--:--|
|Status line.........|
*/
static void lcd_implementation_status_screen()
{
#ifdef TENLOG_CONTROLLER
    //lcd_upadate();
    String strAll = "main.sStatus.txt=\"";
    //TenlogScreen_print("main.sStatus.txt=\"");
    
    long lN = current_position[X_AXIS]*10.0; //1
    String sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = current_position[Y_AXIS]*10.0;     //2
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = current_position[Z_AXIS]*10.0;     //3
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = current_position[E_AXIS]*10.0;     //4
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = int(degTargetHotend(0) + 0.5);     //5
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = int(degHotend(0) + 0.5);           //6
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = int(degTargetHotend(1) + 0.5);     //7
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = int(degHotend(1) + 0.5);           //8
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = int(degTargetBed() + 0.5);         //9
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = int(degBed() + 0.5);               //10
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = fanSpeed;                          //11
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN = feedmultiply;                      //12
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    if (IS_SD_PRINTING)                     //13
    {
        //TenlogScreen_print("1|");
        strAll = strAll +  "1|";
        lN = card.percentDone();
        sSend = String(lN);
        //TenlogScreen_print(sSend);          //14
        //TenlogScreen_print("|");
        strAll = strAll +  sSend + "|";
    }
    else
    {
        //TenlogScreen_print("0|0|");
        strAll = strAll + "0|0|";
    }

    lN=active_extruder;                     //15
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    lN=dual_x_carriage_mode;                //16
    sSend = String(lN);
    //TenlogScreen_print(sSend);
    //TenlogScreen_print("|");
    strAll = strAll +  sSend + "|";

    //lN=dual_x_carriage_mode;                //17 time
    if(IS_SD_PRINTING){
        uint16_t time = millis()/60000 - starttime/60000;
        sSend = String(itostr2(time/60)) + ":" + String(itostr2(time%60));
        //TenlogScreen_print(sSend);
        //TenlogScreen_print("|");
        strAll = strAll +  sSend + "|";
    }else{
        //TenlogScreen_print("00:00|");
        strAll = strAll + "00:00|";    
    }

    if(card.isFileOpen()){              //18 is file open
        strAll = strAll + "1|";
        //ZYF_DEBUG_PRINT_LN("File Opened!");
    }else{
        strAll = strAll + "0|";
        //ZYF_DEBUG_PRINT_LN("File Closed!");
    }

    if(isHeatingHotend(0)){              //19 is heating nozzle 0
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    if(isHeatingHotend(1)){              //20 is heating nozzle 1
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    if(isHeatingBed()){              //21 is heating Bed
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    //TenlogScreen_print("\"");
    strAll = strAll + "\"";
    TenlogScreen_println(strAll);
    //TenlogScreen_printend();
    //ZYF_DEBUG_PRINT_LN(strAll);
    delay(50);
    TenlogScreen_println("click btReflush,0");
    //main.sStatus.txt="1000|800|100|200|180|200|185|35|30|255|0|0|"
#endif
}

static void lcd_implementation_drawmenu_generic(uint8_t row, const char* pstr, char pre_char, char post_char)
{
    char c;
    //Use all characters in narrow LCDs
#if LCD_WIDTH < 20
    uint8_t n = LCD_WIDTH - 1 - 1;
#else
    uint8_t n = LCD_WIDTH - 1 - 2;
#endif
    lcd.setCursor(0, row);
    lcd.print(pre_char);
    while( ((c = pgm_read_byte(pstr)) != '\0') && (n>0) )
    {
        lcd.print(c);
        pstr++;
        n--;
    }
    while(n--)
        lcd.print(' ');
    lcd.print(post_char);
    lcd.print(' ');
}
static void lcd_implementation_drawmenu_setting_edit_generic(uint8_t row, const char* pstr, char pre_char, char* data)
{
    char c;
    //Use all characters in narrow LCDs
#if LCD_WIDTH < 20
    uint8_t n = LCD_WIDTH - 1 - 1 - strlen(data);
#else
    uint8_t n = LCD_WIDTH - 1 - 2 - strlen(data);
#endif
    lcd.setCursor(0, row);
    lcd.print(pre_char);
    while( ((c = pgm_read_byte(pstr)) != '\0') && (n>0) )
    {
        lcd.print(c);
        pstr++;
        n--;
    }
    lcd.print(':');
    while(n--)
        lcd.print(' ');
    lcd.print(data);
}
static void lcd_implementation_drawmenu_setting_edit_generic_P(uint8_t row, const char* pstr, char pre_char, const char* data)
{
    char c;
    //Use all characters in narrow LCDs
#if LCD_WIDTH < 20
    uint8_t n = LCD_WIDTH - 1 - 1 - strlen_P(data);
#else
    uint8_t n = LCD_WIDTH - 1 - 2 - strlen_P(data);
#endif
    lcd.setCursor(0, row);
    lcd.print(pre_char);
    while( ((c = pgm_read_byte(pstr)) != '\0') && (n>0) )
    {
        lcd.print(c);
        pstr++;
        n--;
    }
    lcd.print(':');
    while(n--)
        lcd.print(' ');
    lcd_printPGM(data);
}
#define lcd_implementation_drawmenu_setting_edit_int3_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', itostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_int3(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', itostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float3_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float3(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float32_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr32(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float32(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr32(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float5_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float5(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float52_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr52(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float52(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr52(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float51_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr51(*(data)))
#define lcd_implementation_drawmenu_setting_edit_float51(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr51(*(data)))
#define lcd_implementation_drawmenu_setting_edit_long5_selected(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_long5(row, pstr, pstr2, data, minValue, maxValue) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_bool_selected(row, pstr, pstr2, data) lcd_implementation_drawmenu_setting_edit_generic_P(row, pstr, '>', (*(data))?PSTR(MSG_ON):PSTR(MSG_OFF))
#define lcd_implementation_drawmenu_setting_edit_bool(row, pstr, pstr2, data) lcd_implementation_drawmenu_setting_edit_generic_P(row, pstr, ' ', (*(data))?PSTR(MSG_ON):PSTR(MSG_OFF))

//Add version for callback functions
#define lcd_implementation_drawmenu_setting_edit_callback_int3_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', itostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_int3(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', itostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float3_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float3(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr3(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float32_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr32(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float32(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr32(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float5_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float5(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float52_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr52(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float52(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr52(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float51_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr51(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_float51(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr51(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_long5_selected(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, '>', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_long5(row, pstr, pstr2, data, minValue, maxValue, callback) lcd_implementation_drawmenu_setting_edit_generic(row, pstr, ' ', ftostr5(*(data)))
#define lcd_implementation_drawmenu_setting_edit_callback_bool_selected(row, pstr, pstr2, data, callback) lcd_implementation_drawmenu_setting_edit_generic_P(row, pstr, '>', (*(data))?PSTR(MSG_ON):PSTR(MSG_OFF))
#define lcd_implementation_drawmenu_setting_edit_callback_bool(row, pstr, pstr2, data, callback) lcd_implementation_drawmenu_setting_edit_generic_P(row, pstr, ' ', (*(data))?PSTR(MSG_ON):PSTR(MSG_OFF))


void lcd_implementation_drawedit(const char* pstr, char* value)
{
    lcd.setCursor(1, 1);
    lcd_printPGM(pstr);
    lcd.print(':');
#if LCD_WIDTH < 20
    lcd.setCursor(LCD_WIDTH - strlen(value), 1);
#else
    lcd.setCursor(LCD_WIDTH -1 - strlen(value), 1);
#endif
    lcd.print(value);
    //SERIAL_PROTOCOLPGM("Value: "); SERIAL_PROTOCOLLN(value); // debug By zyf
}
static void lcd_implementation_drawmenu_sdfile_selected(uint8_t row, const char* pstr, const char* filename, char* longFilename)
{
    char c;
    uint8_t n = LCD_WIDTH - 1;
    lcd.setCursor(0, row);
    lcd.print('>');
    if (longFilename[0] != '\0')
    {
        filename = longFilename;
        longFilename[LCD_WIDTH-1] = '\0';
    }
    while( ((c = *filename) != '\0') && (n>0) )
    {
        lcd.print(c);
        filename++;
        n--;
    }
    while(n--)
        lcd.print(' ');
}
static void lcd_implementation_drawmenu_sdfile(uint8_t row, const char* pstr, const char* filename, char* longFilename)
{
    char c;
    uint8_t n = LCD_WIDTH - 1;
    lcd.setCursor(0, row);
    lcd.print(' ');
    if (longFilename[0] != '\0')
    {
        filename = longFilename;
        longFilename[LCD_WIDTH-1] = '\0';
    }
    while( ((c = *filename) != '\0') && (n>0) )
    {
        lcd.print(c);
        filename++;
        n--;
    }
    while(n--)
        lcd.print(' ');
}
static void lcd_implementation_drawmenu_sddirectory_selected(uint8_t row, const char* pstr, const char* filename, char* longFilename)
{
    char c;
    uint8_t n = LCD_WIDTH - 2;
    lcd.setCursor(0, row);
    lcd.print('>');
    lcd.print(LCD_STR_FOLDER[0]);
    if (longFilename[0] != '\0')
    {
        filename = longFilename;
        longFilename[LCD_WIDTH-2] = '\0';
    }
    while( ((c = *filename) != '\0') && (n>0) )
    {
        lcd.print(c);
        filename++;
        n--;
    }
    while(n--)
        lcd.print(' ');
}
static void lcd_implementation_drawmenu_sddirectory(uint8_t row, const char* pstr, const char* filename, char* longFilename)
{
    char c;
    uint8_t n = LCD_WIDTH - 2;
    lcd.setCursor(0, row);
    lcd.print(' ');
    lcd.print(LCD_STR_FOLDER[0]);
    if (longFilename[0] != '\0')
    {
        filename = longFilename;
        longFilename[LCD_WIDTH-2] = '\0';
    }
    while( ((c = *filename) != '\0') && (n>0) )
    {
        lcd.print(c);
        filename++;
        n--;
    }
    while(n--)
        lcd.print(' ');
}
#define lcd_implementation_drawmenu_back_selected(row, pstr, data) lcd_implementation_drawmenu_generic(row, pstr, LCD_STR_UPLEVEL[0], LCD_STR_UPLEVEL[0])
#define lcd_implementation_drawmenu_back(row, pstr, data) lcd_implementation_drawmenu_generic(row, pstr, ' ', LCD_STR_UPLEVEL[0])
#define lcd_implementation_drawmenu_submenu_selected(row, pstr, data) lcd_implementation_drawmenu_generic(row, pstr, '>', LCD_STR_ARROW_RIGHT[0])
#define lcd_implementation_drawmenu_submenu(row, pstr, data) lcd_implementation_drawmenu_generic(row, pstr, ' ', LCD_STR_ARROW_RIGHT[0])
#define lcd_implementation_drawmenu_gcode_selected(row, pstr, gcode) lcd_implementation_drawmenu_generic(row, pstr, '>', ' ')
#define lcd_implementation_drawmenu_gcode(row, pstr, gcode) lcd_implementation_drawmenu_generic(row, pstr, ' ', ' ')
#define lcd_implementation_drawmenu_function_selected(row, pstr, data) lcd_implementation_drawmenu_generic(row, pstr, '>', ' ')
#define lcd_implementation_drawmenu_function(row, pstr, data) lcd_implementation_drawmenu_generic(row, pstr, ' ', ' ')

static void lcd_implementation_quick_feedback()
{
#ifdef LCD_USE_I2C_BUZZER
    lcd.buzz(60,1000/6);
#elif defined(BEEPER) && BEEPER > -1
    SET_OUTPUT(BEEPER);
    for(int8_t i=0;i<10;i++)
    {
        WRITE(BEEPER,HIGH);
        delayMicroseconds(100);
        WRITE(BEEPER,LOW);
        delayMicroseconds(100);
    }
#endif
}

#ifdef LCD_HAS_STATUS_INDICATORS
static void lcd_implementation_update_indicators()
{
#if defined(LCD_I2C_PANELOLU2) || defined(LCD_I2C_VIKI)
    //set the LEDS - referred to as backlights by the LiquidTWI2 library 
    static uint8_t ledsprev = 0;
    uint8_t leds = 0;
    if (target_temperature_bed > 0) leds |= LED_A;
    if (target_temperature[0] > 0) leds |= LED_B;
    if (fanSpeed) leds |= LED_C;
#if EXTRUDERS > 1  
    if (target_temperature[1] > 0) leds |= LED_C;
#endif
    if (leds != ledsprev) {
        lcd.setBacklight(leds);
        ledsprev = leds;
    }
#endif
}
#endif

#ifdef LCD_HAS_SLOW_BUTTONS
static uint8_t lcd_implementation_read_slow_buttons()
{
#ifdef LCD_I2C_TYPE_MCP23017
    // Reading these buttons this is likely to be too slow to call inside interrupt context
    // so they are called during normal lcd_update
    return lcd.readButtons() << B_I2C_BTN_OFFSET; 
#endif
}
#endif

//By zyf
#ifdef SHOW_BOOTSCREEN_2004
static void bootscreen() {
    show_bootscreen = false;
    lcd.clear();

    //long
#if defined(CUSTOM_TENLOG)
    byte Chr0[8] = {0,0,0,0,1,1,0,0}; 
    byte Chr1[8] = {1,1,3,3,31,31,7,6}; 
    byte Chr2[8] = {1,1,0,0,31,31,28,28}; 
    byte Chr3[8] = {0,16,24,16,30,30,0,0}; 
    byte Chr4[8] = {0,0,0,0,1,3,7,14}; 
    byte Chr5[8] = {12,28,25,17,19,15,31,19}; 
    byte Chr6[8] = {25,31,31,28,16,0,1,31}; 
    byte Chr7[8] = {16,0,0,0,0,16,16,0}; 
#else
    byte Chr0[8] = {0,3,7,15,12,12,0,3}; 
    byte Chr1[8] = {0,24,28,30,14,14,14,28}; 
    byte Chr2[8] = {0,31,31,31,24,24,24,24}; 
    byte Chr3[8] = {0,16,24,28,30,30,14,14}; 
    byte Chr4[8] = {3,3,0,12,12,15,7,3}; 
    byte Chr5[8] = {24,28,14,14,14,30,28,24}; 
    byte Chr6[8] = {24,24,24,24,24,31,31,31}; 
    byte Chr7[8] = {14,14,30,28,28,28,24,0}; 
#endif

    lcd.createChar(0, Chr0);
    lcd.createChar(1, Chr1);
    lcd.createChar(2, Chr2);
    lcd.createChar(3, Chr3);
    lcd.setCursor(2, 0);
    lcd.print('\x0');
    lcd.print('\x1');
    lcd.print('\x2');
    lcd.print('\x3');
    lcd.createChar(4, Chr4);
    lcd.createChar(5, Chr5);
    lcd.createChar(6, Chr6);
    lcd.createChar(7, Chr7);
    lcd.setCursor(2, 1);
    lcd.print('\x4');
    lcd.print('\x5');
    lcd.print('\x6');
    lcd.print('\x7');

#if defined(CUSTOM_NONE)
    lcd.setCursor(7, 0); lcd_printPGM(PSTR("+----------+" ));
    lcd.setCursor(7, 1); lcd_printPGM(PSTR("|D3 PRINTER|"));
    lcd.setCursor(7, 2); lcd_printPGM(PSTR("+----------+" ));
    lcd.setCursor(6, 3); lcd_printPGM(PSTR("WELCOME!" ));
#elif defined(CUSTOM_TENLOG)
    lcd.setCursor(7, 0); lcd_printPGM(PSTR("+----------+" ));
    lcd.setCursor(7, 1); lcd_printPGM(PSTR("|TENLOG  D3|"));
    lcd.setCursor(7, 2); lcd_printPGM(PSTR("+----------+" ));
    lcd.setCursor(3, 3); lcd_printPGM(PSTR("www.tenlog.cn" ));
#elif defined(CUSTOM_HICTOP)
    lcd.setCursor(7, 0); lcd_printPGM(PSTR("+----------+" ));
    lcd.setCursor(7, 1); lcd_printPGM(PSTR("|  HICTOP  |"));
    lcd.setCursor(7, 2); lcd_printPGM(PSTR("+----------+" ));
    lcd.setCursor(1, 3); lcd_printPGM(PSTR("hictop3dprinter.com" ));
#endif

    delay(5000);
    //bootscreen1();
}

#endif

#endif//ULTRA_LCD_IMPLEMENTATION_HITACHI_HD44780_H
