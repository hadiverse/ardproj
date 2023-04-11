#include <SD.h>                   //this library included to could use SD card module
#include <TMRpcm.h>               //this library included to play sounds".wav" on speaker
#include <Wire.h>                 //this library included to could use I2C interface with LCD
#include <LiquidCrystal_I2C.h>    //this library included to work on LCD which depend on I2C interface in transmitting and receiving data

#define SD_ChipSelectPin   8     //define chip selected pin of SD card module 

#define pin1  3                   //these are the Arduino pins that we use to activate coils 1-4 of the stepper motor
#define pin2  4
#define pin3  5
#define pin4  6

#define delaytime 12             //delay time in ms to control the stepper motor delaytime.
                                 //Our tests showed that 8 is about the fastest that can yield reliable operation w/o missing steps
#define counts_per_pill  35

#define hours   0                //this define related to location byte of hours in EEPROM
#define minutes 1                //this define related to location byte of minutes in EEPROM
#define AM_PM   2                //this define related to loaction of AM/PM byte in EEPROM
#define AM      0
#define PM      1

#define connecting    'C'
#define disconnecting 'D'
#define cancel        'X'
#define set           'S'
#define Exit          'e'

#define pill_1   'a'
#define P1_state  5
#define P1_hour   6
#define P1_minute 7
#define P1_AM_PM  8
#define pill_2    'b'
#define P2_state  10
#define P2_hour   11
#define P2_minute 12
#define P2_AM_PM  13
#define pill_3    'c'
#define P3_state  15
#define P3_hour   16
#define P3_minute 17
#define P3_AM_PM  18
#define pill_4    'd'
#define P4_state  20
#define P4_hour   21
#define P4_minute 22
#define P4_AM_PM  23
#define pill_5    'e'
#define P5_state  25
#define P5_hour   26
#define P5_minute 27
#define P5_AM_PM  28
#define pill_6    'f'
#define P6_state  30
#define P6_hour   31
#define P6_minute 32
#define P6_AM_PM  33
#define pill_7    'g'
#define P7_state  35
#define P7_hour   36
#define P7_minute 37
#define P7_AM_PM  38
#define pill_8    'h'
#define P8_state  40
#define P8_hour   41
#define P8_minute 42
#define P8_AM_PM  43
#define pill_9    'k'
#define P9_state  45
#define P9_hour   46
#define P9_minute 47
#define P9_AM_PM  48
#define pill_10   'l'
#define P10_state  50
#define P10_hour   51
#define P10_minute 52
#define P10_AM_PM  53
#define pill_11   'm'
#define P11_state  55
#define P11_hour   56
#define P11_minute 57
#define P11_AM_PM  58
#define pill_12   'n'
#define P12_state 60
#define P12_hour 61
#define P12_minute 62
#define P12_AM_PM 63
#define pill_13 'o'
#define P13_state 65
#define P13_hour 66
#define P13_minute 67
#define P13_AM_PM 68
#define P14_hour   71
#define P14_minute 72
#define P14_AM_PM  73

#define empty       0
#define setted      1

TMRpcm tmrpcm;                         // create object for speaker device
LiquidCrystal_I2C lcd(0x27,16,2);      // create object named by lcd to use it later during working on lcd
                                       // first argment is the address of lcd which is 0x27
                                       // second argument is the number of columns which is 16
                                       // third argument is the number of row which is 2

void welcome_message(void);            // this function to show welcome message
void wait_app_connecting(void);        // this function used to run in void setup only for one time to wait connecting to 
                                       // application to set the time because setting time is very important point and pill
                                       // does not work until user set the time
void wait_setting_time(void);          // this function used to get time from user through app then save it in EEPROM to reuse later
void start_time_counting(void);        // this function used to start counting of time
void device_ready_to_work(void);       // this function to tell the user that the device is ready to use 
void time_showing(void);               // this function show current time on LCD
void time_setting(char pill_hour_location); // this function used to get time of cells from user then set it in specified cell space
int app_command(void);                 // this function return last comman received from app
char check_pill_state(char pill);      // this function used to check if pill cell is setted or not 
void check_previous_pill(char pill);  // this function used to check if previous cell is setted or not
void stepper_moving(void);             // this function used to rotate stepper motor
void Step_A(void);                     // the following four functionts related to motion of stepper motor
void Step_B(void);
void Step_C(void);
void Step_D(void);
void forward(void);

volatile int counter=0;                // this counter related to time calculation process
volatile char Minutes, Hours, carry, ampm; // these global variables used inside timer interrupt for temporary saving and manipulations
volatile byte memory[100]={0};         // this array of 100 byte to save time and state of each pill 
char app_state='C';                    // this variable used as indicator to the state of android application to know if it connected or disconnected

void setup() {
  // put your setup code here, to run once:
  
  lcd.init();                          // initialize the lcd
                                       // Note: This .init() method also starts the I2C bus, i.e. there does not need to be
                                       // a separate "Wire.begin();" statement in the setup.
  lcd.backlight();                     // turn on the backlight of LCD
  
  Serial.begin(9600);                  // set baud rate of UART communication to 9600
 
  pinMode(pin1, OUTPUT);               // the next four lines to define pins which used to control stepper motor
  pinMode(pin2, OUTPUT); 
  pinMode(pin3, OUTPUT); 
  pinMode(pin4, OUTPUT); 

if (!SD.begin(SD_ChipSelectPin)) return;
tmrpcm.speakerPin = 9; tmrpcm.setVolume(6);
welcome_message(); wait_app_connecting(); wait_setting_time(); start_time_counting(); device_ready_to_work();

  
void loop() {
  // put your main code here, to run repeatedly:
  int command = app_command();
  int state = app_state;
  while (state == connecting) {
    for (char i = pill_1; i <= pill_14; i++) {
      if (command == i) {
        if (check_pill_state(i - pill_1 + 1))
          check_previous_pill(i - pill_1 + 1);
        do {
          command = app_command();
          state = app_state;
        } while (!(command == cancel || command == set || state == disconnecting));
        if (command == cancel || command == disconnecting) {
          time_showing();
        } else if (command == set) {
          time_setting(i - pill_1 + 1);
        }
        break;
      }
    }
    command = app_command();
    state = app_state;
  }
  time_showing();
  delay(100);
}

void start_time_counting(void) {
  noInterrupts();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 99;
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
  TIMSK2 |= (1 << TOIE0);
  interrupts();
}

void device_ready_to_work(void){

  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("your pill is");
  lcd.setCursor(2,1);
  lcd.print("ready to work");
  tmrpcm.play("ready.wav");
  delay(2000);
}

void welcome_message(void) {
  lcd.setCursor(0,0);
  lcd.print("welcome to pill");
  lcd.setCursor(2,1);
  lcd.print("dispenser");
  tmrpcm.play("welcome.wav");
  delay(2000);
}

void wait_app_connecting(void) {
  tmrpcm.play("app.wav");
  lcd.setCursor(0,0);
  lcd.print("  connect app");
  lcd.setCursor(2,1);
  lcd.print("to start");
  delay(2000);
}

void wait_setting_time(void){
  int data_array[3];      // create an array to save received time data
  char ap[3];             // create an array to save AM/PM data
  
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("time setting");
  tmrpcm.play("settingTime.wav");

  // read hours, minutes, and AM/PM from the serial monitor
  while (Serial.available() < 7);
  for (int i = 0; i < 7; i++) {
    char c = Serial.read();
    if (i == 0 || i == 1) {     // read hours
      data_array[0] *= 10;
      data_array[0] += (c - '0');
    } else if (i == 3 || i == 4) {  // read minutes
      data_array[1] *= 10;
      data_array[1] += (c - '0');
    } else if (i == 5 || i == 6) {  // read AM/PM
      ap[i-5] = c;
    }
  }

  // calculate hours based on AM/PM
  if (ap[0] == 'P' && data_array[0] != 12) {
    data_array[0] += 12;
  } else if (ap[0] == 'A' && data_array[0] == 12) {
    data_array[0] = 0;
  }

  // update memory and display
  memory[hours] = data_array[0];
  memory[minutes] = data_array[1];
  memory[AM_PM] = (ap[0] == 'A') ? AM : PM;
  lcd.setCursor(4,1);
  lcd.print(memory[hours] < 10 ? "0" : "");
  lcd.print(memory[hours]);
  lcd.print(":");
  lcd.print(memory[minutes] < 10 ? "0" : "");
  lcd.print(memory[minutes]);
  lcd.print(" ");
  lcd.print(memory[AM_PM] == AM ? "AM" : "PM");
  delay(2000);
}

void time_showing(void){
  static char counter=5;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Time ");
  lcd.setCursor(8,0);
  lcd.print(memory[hours]<10 ? "0" : "");
  lcd.print(memory[hours]);
  lcd.print(":");
  lcd.print(memory[minutes]<10 ? "0" : "");
  lcd.print(memory[minutes]);
  lcd.print(" ");
  lcd.print(memory[AM_PM] == AM ? "AM" : "PM");
  
  if(memory[counter] == 1){
    lcd.setCursor(0,1);
    lcd.print("next");
    lcd.setCursor(8,1);
    lcd.print(memory[counter+1]<10 ? "0" : "");
    lcd.print(memory[counter+1]);
    lcd.print(":");
    lcd.print(memory[counter+2]<10 ? "0" : "");
    lcd.print(memory[counter+2]);
    lcd.print(" ");
    lcd.print(memory[counter+3] == AM ? "AM" : "PM");

    if(memory[hours] == memory[counter+1] && memory[minutes] == memory[counter+2] && memory[AM_PM] == memory[counter+3]){
      if(counter>=10) memory[counter-5]=0;
      lcd.clear();
      lcd.print("pill outting now");
      lcd.setCursor(3,1);
      lcd.print(memory[counter+1]<10 ? "0" : "");
      lcd.print(memory[counter+1]);
      lcd.print(":");
      lcd.print(memory[counter+2]<10 ? "0" : "");
      lcd.print(memory[counter+2]);
      lcd.print(" ");
      lcd.print(memory[counter+3] == AM ? "AM" : "PM");
      tmrpcm.play("Dr.wav");
      stepper_moving();
      counter = (counter+5)%75;
      if(counter == 0) counter = 5;
    }
  } else {
    lcd.setCursor(0,1);
    lcd.print("next");
    lcd.setCursor(8,1);
    lcd.print("--:-- --");
  }
}

void time_setting(char pill_hour_location){
  int data_array[3] = {0}; // Initialize an array to store the hour, minute, and AM/PM data
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("time setting");
  tmrpcm.play("pillTime.wav");
  for (int i = 0; i < 7; i++) { // Read up to 7 characters from serial input
    while (!Serial.available()); // Wait for data to become available
    char reading = Serial.read();
    if (reading == Exit || reading == cancel) return; // Exit the function if the user cancels or exits
    if (i == 1 || i == 4) { // Skip the ':' character
      i++;
      continue;
    }
    data_array[i/2] = data_array[i/2] * 10 + (reading - '0'); // Convert the character to an integer and add it to the appropriate array element
    lcd.setCursor(4 + (i/2)*3, 1); // Update the time display on the LCD
    if (i == 5) {
      lcd.print(reading == 'A' ? "AM" : "PM");
    } else {
      if (data_array[i/2] < 10) lcd.print(0);
      lcd.print(data_array[i/2]);
    }
  }
  memory[pill_hour_location-1] = 1; // Set the state of this pill to 1
  memory[pill_hour_location] = data_array[0]; // Store the hour, minute, and AM/PM data in the appropriate memory locations
  memory[pill_hour_location+1] = data_array[1];
  memory[pill_hour_location+2] = (data_array[2] == 'P') ? PM : AM;
  lcd.setCursor(4, 1); // Update the time display on the LCD
  if (memory[pill_hour_location] < 10) lcd.print(0);
  lcd.print(memory[pill_hour_location]);
  lcd.print(":");
  if (memory[pill_hour_location+1] < 10) lcd.print(0);
  lcd.print(memory[pill_hour_location+1]);
  lcd.print(" ");
  lcd.print(memory[pill_hour_location+2] == AM ? "AM" : "PM");
  delay(2000);
}

int app_command(void){
  if(Serial.available()){
    char reading = Serial.read();
    app_state = (reading == disconnecting) ? disconnecting : connecting;
    return reading;
  }
  return 0;
}

char check_pill_state(char pill){
  char reading = memory[pill];
  if(reading == empty){
    lcd.clear();
    lcd.print("this cell is empty");
    tmrpcm.play("empty.wav");
  }else{
    lcd.clear();
    lcd.print("setted before at");
    lcd.setCursor(2,1);
    if(memory[pill+1]<10)
      lcd.print(0);
    lcd.print(memory[pill+1]);
    lcd.print(":");
    if(memory[pill+2]<10)
      lcd.print(0);
    lcd.print(memory[pill+2]);
    lcd.print(" ");
    lcd.print((memory[pill+3] == AM) ? "AM" : "PM");
    tmrpcm.play("setted.wav");
  }
  delay(2000);
  return (reading == empty) ? 1 : 0;
}

void check_previous_pill(char pill){
  int available_pill = pill - 5;
  while (available_pill > 0 && memory[available_pill] == 0) {
    available_pill -= 5;
  }
  if (pill > 5 && memory[pill - 5] == 0) {
    lcd.clear();
    lcd.print("you need to set");
    lcd.setCursor(1, 1);
    lcd.print("pill ");
    tmrpcm.play("previous.wav");
    lcd.print((available_pill + 5) / 5);
    lcd.print(" first");
    delay(4000);
  }
}


void stepper_moving(void){
  for(int counter=0; counter<counts_per_pill; counter++){
    forward();
  }
}
void Step_A(){
  digitalWrite(pin1, HIGH);//turn on coil 1 
  digitalWrite(pin2, LOW); 
  digitalWrite(pin3, LOW); 
  digitalWrite(pin4, LOW); 
}
void Step_B(){
  digitalWrite(pin1, LOW); 
  digitalWrite(pin2, HIGH);//turn on coil 2
  digitalWrite(pin3, LOW); 
  digitalWrite(pin4, LOW); 
}
void Step_C(){
  digitalWrite(pin1, LOW); 
  digitalWrite(pin2, LOW); 
  digitalWrite(pin3, HIGH); //turn on coil 3
  digitalWrite(pin4, LOW); 
}
void Step_D(){
  digitalWrite(pin1, LOW); 
  digitalWrite(pin2, LOW); 
  digitalWrite(pin3, LOW); 
  digitalWrite(pin4, HIGH); //turn on coil 4
}
void step_OFF(){
  digitalWrite(pin1, LOW); //power all coils down
  digitalWrite(pin2, LOW); 
  digitalWrite(pin3, LOW); 
  digitalWrite(pin4, LOW); 
}

void forward(){//one tooth forward
  Step_A();
  delay(delaytime);
  Step_B();
  delay(delaytime);
  Step_C();
  delay(delaytime);
  Step_D();
  delay(delaytime);
}

ISR(TIMER2_OVF_vect){                 
  TCNT2 = 99;
  if(++counter == 6000){ // Using pre-increment to avoid a copy of the variable
    counter = 0;
    if(++memory[minutes] == 60) {
      memory[minutes] = 0;
      if(++memory[hours] == 12) {
        memory[AM_PM] ^= 1;
      }
      if(memory[hours] == 0) {
        memory[hours] = 12;
      }
    }
  }
}









