//
// Created by noodles on 25-2-19.
//

#include "servo.h"

int main() {

    Servo servo("/dev/ttyS0", 115200, 0, 16);
    servo.init();

    servo.setMode(1, 0);
    servo.setAngle(1, 0, 0);
    servo.setSpeed(1, 0);

    int pos = servo.getPosition(1);
    printf("Servo position: %d\n", pos);

    servo.close();

    return 0;
}