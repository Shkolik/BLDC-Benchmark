const char s_motorType[] PROGMEM = "MOTOR TYPE";
const char s_motor14p[] PROGMEM = "12S14P";
const char s_motor12p[] PROGMEM = "9S12P";

const char s_bladesCount[] PROGMEM = "BLADES COUNT";
const char s_edit[] PROGMEM = " <           >  ";
const char s_enableScale[] PROGMEM = "THRUST SENSOR";
const char s_enableScale_true[] PROGMEM = "ENABLED";
const char s_enableScale_false[] PROGMEM = "DISABLED";

const char s_rpm[] PROGMEM = "RPM:";

const char s_mode[] PROGMEM = "SELECT MODE";
const char s_mode_stand[] PROGMEM = "CONNECTED";
const char s_mode_hh[] PROGMEM = "HANDHELD";

//main screen
const char s_header1[] PROGMEM = "RPM:  VOLT: KV:";
const char s_header2[] PROGMEM = "THRUST:  A:";
const char s_header3[] PROGMEM = "THROTTLE:";

const char *const l_headers[] PROGMEM = {s_header1, s_header2, s_header3};
const char *const l_settings_stand[] PROGMEM = {s_mode, s_motorType, s_enableScale};
const char *const l_settings_hh[] PROGMEM = {s_mode, s_bladesCount, s_enableScale};
const char *const l_motorTypes[] PROGMEM = {s_motor14p, s_motor12p};
const char *const l_modes[] PROGMEM = {s_mode_hh, s_mode_stand};
const char *const l_enableScale_options[] PROGMEM = {s_enableScale_false, s_enableScale_true};


// 3blades top right
byte customChar0[] = {
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B00000,
  B10000
};

// 3blades top left
byte customChar1[] = {
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00000,
  B00001
};

// 3blades bottom left
byte customChar2[] = {
  B00001,
  B00000,
  B00110,
  B00100,
  B01100,
  B01000,
  B11000,
  B10000
};

// 3blades bottom right
byte customChar3[] = {
  B10000,
  B00000,
  B01100,
  B00100,
  B00110,
  B00010,
  B00011,
  B00001
};

// 4blades top right
byte customChar4[] = {
  B00001,
  B00011,
  B00010,
  B00110,
  B00100,
  B01100,
  B00000,
  B10000
};

// 4blades top left
byte customChar5[] = {
  B10000,
  B11000,
  B01000,
  B01100,
  B00100,
  B00110,
  B00000,
  B00001
};

// 2blades left top
byte customChar6[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00001
};

// 2blades right bottom
byte customChar7[] = {
  B10000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

char pgm_string_buff[17];

char* readPgmString(uint16_t ptr)
{  
  uint8_t i = 0;
  do
  {
    pgm_string_buff[i] = (char)pgm_read_byte(ptr++);
  }
  while(pgm_string_buff[i++] != NULL);

  return pgm_string_buff;
}
