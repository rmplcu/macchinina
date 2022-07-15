#include <NewPing.h>
#include <IRremote.hpp>
#include <TaskScheduler.h>

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

Scheduler scheduler;

NewPing sonar_front_left(TRIG_FRONT_LEFT, ECHO_FRONT_LEFT, MAX_DIST);
NewPing sonar_left(TRIG_LEFT, ECHO_LEFT, MAX_DIST);
NewPing sonar_front_right(TRIG_FRONT_RIGHT, ECHO_FRONT_RIGHT, MAX_DIST);
NewPing sonar_right(TRIG_RIGHT, ECHO_RIGHT, MAX_DIST);

byte sp_right = BASE_SPEED;
byte sp_left = BASE_SPEED;


//Set motors direction to move forward
void forward() {
  digitalWrite(DIRECTION_LEFT, HIGH);
  digitalWrite(DIRECTION_RIGHT, HIGH);

  digitalWrite(REVERSE_DIRECTION_LEFT, LOW);
  digitalWrite(REVERSE_DIRECTION_RIGHT, LOW);
}

//Set motors direction to move backward
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


void follow_left() {
  float dist_left = calc_distance_cm(sonar_left, MAX_DIST);
  float dist_front = calc_distance_cm(sonar_front_left, MAX_DIST);

  if (dist_front <= 15) {
    turn_right();
  } else {
    forward();
    if (dist_left < 4) {
      sp_left = 255;
      sp_right = 0;
    } else if (dist_left > 20) {
      sp_left = 0;
      sp_right = 255;
    } else {
      sp_left = BASE_SPEED + (10 - dist_left) * 3.5;
      sp_right = BASE_SPEED;
    }

    move_motors(sp_left, sp_right);
  }
}

//Follow wall from right side of the car
void follow_right() {
  float dist_right = calc_distance_cm(sonar_right, MAX_DIST);
  float dist_front = calc_distance_cm(sonar_front_right, MAX_DIST);

  if (dist_front <= 15) {
    turn_left();
  } else {
    forward();
    if (dist_right < 4) {
      sp_right = 255;
      sp_left = 0;
    } else if (dist_right > 20) {
      sp_right = 0;
      sp_left = 255;
    } else {
      sp_left = BASE_SPEED;
      sp_right = BASE_SPEED + (10 - dist_right) * 3.5;
    }

    move_motors(sp_left, sp_right);
  }
}

//Go forward till wall
void reach_wall() {
  forward();
  if (calc_distance_cm(sonar_front_left, MAX_DIST) >= 15 && calc_distance_cm(sonar_front_right, MAX_DIST) >= 15 ) {
    move_motors(255,255);
  } else {
    move_motors(0,0);
    digitalWrite(LED_G, LOW); 
    scheduler.disableAll();
  }

}

//Handle joystick commands
void handle_joy(uint16_t command) {
  if (command == 0x5a) {
    forward();
    move_motors(255, 0);
  } else if (command == 0x18) {
    forward();
    move_motors(255,255);
  } else if (command == 0x52) {
    backward();
    move_motors(255,255);
  } else if (command == 0x8) {
    forward();
    move_motors(0, 255);
  } else if (command == 0x1c) {
    move_motors(0,0);
  }
}


Task LeftFollow(20, TASK_FOREVER, follow_left);
Task RightFollow(20, TASK_FOREVER, follow_right);
Task ReachWall(20, TASK_FOREVER, reach_wall);


void setup() {
  Serial.begin(115200);
  //Add task
  scheduler.init();
  scheduler.addTask(LeftFollow);
  scheduler.addTask(RightFollow);
  scheduler.addTask(ReachWall);
  
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
  
  //Enable IR In
  IrReceiver.begin(IR_REC);

}

boolean joymode = false; //joystick mode active?
boolean active = false; //any task enebled? 

void loop() {
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    digitalWrite(LED_R, LOW);

    if (IrReceiver.decodedIRData.command == 0x45 && !active)  {
      digitalWrite(LED_G, HIGH);
      ReachWall.enable();
      active = true;
    } else if (IrReceiver.decodedIRData.command == 0x9 && !active) {
      digitalWrite(LED_B, HIGH);
      LeftFollow.enable();
      active = true;
    } else if (IrReceiver.decodedIRData.command == 0x7 && !active) {
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, HIGH);
      RightFollow.enable();
      active = true;
    } else if (IrReceiver.decodedIRData.command == 0x47 && !active) {
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, HIGH);
      digitalWrite(LED_R, HIGH);
      joymode = true;
      active = true;
    } else if (IrReceiver.decodedIRData.command == 0x40) {
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      digitalWrite(LED_R, LOW);
      scheduler.disableAll();
      joymode = false;
      active = false;
      move_motors(0,0);
    } else if (joymode) {
      handle_joy(IrReceiver.decodedIRData.command);
    } else {
      digitalWrite(LED_R, HIGH);
      Serial.println("no function associated");
    }
  }
  scheduler.execute();
}
