#include <NewPing.h>
#include <IRremote.hpp>

//base "speed", a.k.a. PWM value
#define BASE_SPEED 180

//Directions and PWM of the left motor
#define MOTOR_LEFT 5
#define DIRECTION_LEFT 4
#define REVERSE_DIRECTION_LEFT 6

//Directions and PWM of the right motor
#define MOTOR_RIGHT 9
#define DIRECTION_RIGHT 7
#define REVERSE_DIRECTION_RIGHT 2

//Front-left ultrasonic sensor
#define TRIG_FRONT_LEFT 8
#define ECHO_FRONT_LEFT 12

//Left ultrasonic sensor
#define TRIG_LEFT 10
#define ECHO_LEFT 13

//Front-right ultrasonic sensor
#define TRIG_FRONT_RIGHT A2
#define ECHO_FRONT_RIGHT A3

//Right ultrasonic sensor
#define TRIG_RIGHT A0
#define ECHO_RIGHT A1

//IR sensor receiver
#define IR_REC A4

//Buzzer
#define BUZZ A5

//max distance reached by ping (cm) 
#define MAX_DIST 200

NewPing sonar_front_left(TRIG_FRONT_LEFT, ECHO_FRONT_LEFT, MAX_DIST);
NewPing sonar_left(TRIG_LEFT, ECHO_LEFT, MAX_DIST);
NewPing sonar_front_right(TRIG_FRONT_RIGHT, ECHO_FRONT_RIGHT, MAX_DIST);
NewPing sonar_right(TRIG_RIGHT, ECHO_RIGHT, MAX_DIST);

IRrecv receiver(IR_REC);

byte right_motor_pwm = BASE_SPEED;
byte left_motor_pwm = BASE_SPEED;

void setup() {
  Serial.begin(115200);
  
  //Set direction of motors
  pinMode(DIRECTION_LEFT, OUTPUT);
  pinMode(DIRECTION_RIGHT, OUTPUT);
  pinMode(REVERSE_DIRECTION_LEFT, OUTPUT);
  pinMode(REVERSE_DIRECTION_RIGHT, OUTPUT);
  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);

  //Initial state: both motors off
  digitalWrite(DIRECTION_RIGHT, LOW);
  digitalWrite(DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_RIGHT, LOW);
  
  //Enable IR In
  receiver.enableIRIn();
}

//Set motors direction to move forward
void forward() {
  //left motor goes forward
  digitalWrite(DIRECTION_LEFT, HIGH);
  digitalWrite(DIRECTION_RIGHT, HIGH);

  //right motor goes forward
  digitalWrite(REVERSE_DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_RIGHT, LOW);
}

//Set motors direction to make a right turn 
void turn_right() {
  //right motor goes backwards
  digitalWrite(DIRECTION_RIGHT, LOW);
  digitalWrite(REVERSE_DIRECTION_RIGHT, HIGH);

  //left motor goes forward
  digitalWrite(DIRECTION_LEFT, HIGH);
  digitalWrite(REVERSE_DIRECTION_LEFT, LOW);

  move_motors(255, 255);

  //use time to turn 90 degree (not so good)
  delay(250);
}

//Set duty cycle of PWM of left and right motor to value_sx and value_dx
void move_motors(byte value_sx, byte value_dx) {
  analogWrite(MOTOR_LEFT, value_sx);
  analogWrite(MOTOR_RIGHT, value_dx);
}

//get distance in cm of wall and sonar
float calc_distance_cm(NewPing sonar, int max_dist) {
  float t = sonar.ping_median();
  //57 micros for 2cm: 1cm ping + 1cm echo 
  return t == 0 ? max_dist : (t + 28.5)/57.0;
}

//Follow wall on the left
void follow_left() {
  byte sp_left = BASE_SPEED;
  byte sp_right = BASE_SPEED;
  
  while (true) {
    if (calc_distance_cm(sonar_front_left, MAX_DIST) <= 15) {
        turn_right();
    } else {
      if (calc_distance_cm(sonar_left, MAX_DIST) < 10) {
        sp_left += 20;
      } else if (calc_distance_cm(sonar_left, MAX_DIST) > 15) {
        sp_left -= 20;
      }
      move_motors(sp_left, sp_right);
    }

    if (receiver.decode()) {
      if (receiver.decodedIRData.command == 0x40) {
          move_motors(0,0);
          
          tone(BUZZ, 1175, 500);
          tone(BUZZ, 1397, 500);
          
          break;
      }
      receiver.resume();
    }
    
    delay(10);
  }
}

uint16_t code = 0;

void loop() { 
  if (receiver.decode()) {
     code = receiver.decodedIRData.command;
     receiver.resume();
     if (code == 0x45)  {
        forward();
        while (calc_distance_cm(sonar_front_left, MAX_DIST) >= 15 && calc_distance_cm(sonar_front_right, MAX_DIST) >= 15 ) {
          move_motors(255,255);
          delay(10);
        }
        move_motors(0,0);
        tone(BUZZ, 3322, 200);
        tone(BUZZ, 622, 200);
        tone(BUZZ, 3322, 200);
     } else if (code == 0x9) {
        follow_left();
     } else {
        Serial.println(code, HEX);
     }
  }
  
  delay(60);
}
