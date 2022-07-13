#include <NewPing.h>
#include <IRremote.hpp>

//base "speed", a.k.a. PWM value
#define BASE_SPEED 175

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

//Feedback to user with leds
#define LED_G 11
#define LED_B 3
#define LED_R A5

//max distance reached by ping (cm) 
#define MAX_DIST 200

NewPing sonar_front_left(TRIG_FRONT_LEFT, ECHO_FRONT_LEFT, MAX_DIST);
NewPing sonar_left(TRIG_LEFT, ECHO_LEFT, MAX_DIST);
NewPing sonar_front_right(TRIG_FRONT_RIGHT, ECHO_FRONT_RIGHT, MAX_DIST);
NewPing sonar_right(TRIG_RIGHT, ECHO_RIGHT, MAX_DIST);

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

  pinMode(LED_B, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);

  //Initial state: both motors off
  digitalWrite(DIRECTION_RIGHT, LOW);
  digitalWrite(DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_RIGHT, LOW);
  
  //Enable IR In
  IrReceiver.begin(IR_REC);

}

//Set motors direction to move forward
void forward() {
  digitalWrite(DIRECTION_LEFT, HIGH);
  digitalWrite(DIRECTION_RIGHT, HIGH);

  digitalWrite(REVERSE_DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_RIGHT, LOW);
}

//Set motors direction to move BACKWARD
void backward() {
  digitalWrite(DIRECTION_LEFT, LOW);
  digitalWrite(DIRECTION_RIGHT, LOW);

  digitalWrite(REVERSE_DIRECTION_LEFT, HIGH);
  digitalWrite(REVERSE_DIRECTION_RIGHT, HIGH);
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

void turn_left() {
  //right motor goes forward
  digitalWrite(DIRECTION_RIGHT, HIGH);
  digitalWrite(REVERSE_DIRECTION_RIGHT, LOW);

  //left motor goes backward
  digitalWrite(DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_LEFT, HIGH);

  move_motors(255, 255);

  delay(250);
}

//Set duty cycle of PWM of left and right motor to value_sx and value_dx
void move_motors(byte value_sx, byte value_dx) {
  analogWrite(MOTOR_LEFT, value_sx);
  analogWrite(MOTOR_RIGHT, value_dx);
}

//get distance in cm between wall and sonar
float calc_distance_cm(NewPing sonar, int max_dist) {
  float t = sonar.ping_median();
  //57 micros for 2cm: 1cm ping + 1cm echo 
  return t == 0 ? max_dist : (t + 28.5)/57.0;
}

//Follow wall
void follow(boolean left) {
 
  byte sp_left = BASE_SPEED;
  byte sp_right = BASE_SPEED;

  while (true) {
    //distance left/right
    float d_rl;
    left ? d_rl = calc_distance_cm(sonar_left, MAX_DIST) : d_rl = calc_distance_cm(sonar_right, MAX_DIST);

    //distance front-left/front-right
    float d_f;
    left ? d_f = calc_distance_cm(sonar_front_left, MAX_DIST) : d_f = calc_distance_cm(sonar_front_right, MAX_DIST);

    if (d_f <= 15) {
        left ? turn_right() : turn_left();
    } else {
      forward();
      if (d_rl < 4) {
        left ? sp_left = 255 : sp_left = 0;
        left ? sp_right = 0 : sp_right = 255;
      } else if (d_rl > 20) {
        left ? sp_left = 0 : sp_right = 255;
        left ? sp_right = 255 : sp_right = 0;
      } else {
        left ? sp_left = BASE_SPEED + (10 - d_rl) * 3.5 : sp_left = BASE_SPEED;
        left ? sp_right = BASE_SPEED : sp_right = BASE_SPEED + (10 - d_rl) * 3.5;
      }
      move_motors(sp_left, sp_right);
    }

    if (IrReceiver.decode()) {
      IrReceiver.resume();
      if (IrReceiver.decodedIRData.command == 0x40) {
          move_motors(0,0);       
          break;
      }
    }
    
    delay(30);
  }
}

//Go forward till wall
void reach_wall() {
  forward();
  while (calc_distance_cm(sonar_front_left, MAX_DIST) >= 15 && calc_distance_cm(sonar_front_right, MAX_DIST) >= 15 ) {
    move_motors(255,255);
    delay(10);
  }
  move_motors(0,0);

}

void joystick() {
  while (true) {
    if (IrReceiver.decode()) {
      IrReceiver.resume();
      if (IrReceiver.decodedIRData.command == 0x40) {
        move_motors(0,0);
        return;
      } else if (IrReceiver.decodedIRData.command == 0x5a) {
        forward();
        move_motors(255, 0);
      } else if (IrReceiver.decodedIRData.command == 0x18) {
        forward();
        move_motors(255,255);
      } else if (IrReceiver.decodedIRData.command == 0x52) {
        backward();
        move_motors(255,255);
      } else if (IrReceiver.decodedIRData.command == 0x8) {
        forward();
        move_motors(0, 255);
      } else if (IrReceiver.decodedIRData.command == 0x1c) {
        move_motors(0,0);
      }
    }
  }
  
}


void loop() { 
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    Serial.println(IrReceiver.decodedIRData.command);
    Serial.println(IrReceiver.decodedIRData.protocol);

    if (IrReceiver.decodedIRData.command == 0x45)  {
      digitalWrite(LED_G, HIGH);
      reach_wall();
    } else if (IrReceiver.decodedIRData.command == 0x9) {
      digitalWrite(LED_B, HIGH);
      follow(true);
    } else if (IrReceiver.decodedIRData.command == 0x7) {
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, HIGH);
      follow(false);
    } else if (IrReceiver.decodedIRData.command == 0x47) {
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, HIGH);
      digitalWrite(LED_R, HIGH);
      joystick();
    } else {
      digitalWrite(LED_R, HIGH);
      Serial.println("no function associated");
    }
    digitalWrite(LED_B, LOW);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_R, LOW);
  }
  
  delay(60);
}
