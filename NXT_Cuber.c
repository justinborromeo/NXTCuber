// References to the motors in case we switch them up
const int BASE_MOTOR = motorA;
const int CLAMP_MOTOR = motorB;
const int COLOR_MOTOR = motorC;

// Returns true if any of the motors are active
bool motorsRunning() {
    return (nMotorRunState[BASE_MOTOR] == runStateRunning)
           || (nMotorRunState[CLAMP_MOTOR] == runStateRunning)
           || (nMotorRunState[COLOR_MOTOR] == runStateRunning);
}

// Rotate a motor with a PID controller
void rotatePID(int motor0, int setPoint, int maxTime,
               float K_p, float K_i, float K_d,
               bool CW, int dt) {
    nMotorPIDSpeedCtrl[motor0] = mtrSpeedReg;
    int dir = 1; // invert for counterclockwise
    if (!CW) {
        dir *= -1;
    }
    nMotorEncoder[motor0] = 0;
    float lastError = setPoint;
    float integral = 0;
    float derivative = 0;
    float totalTime = 0;
    while(totalTime < maxTime) { // user defines max spin time
        float u = K_p * lastError + K_i * integral + K_d * derivative;
        motor[motor0] = dir * u;
        wait1Msec(dt);
        float error = setPoint - dir * nMotorEncoder[motor0];
        integral += error * (dt / 1000.0);
        derivative = (error - lastError)/(dt / 1000.0);
        lastError = error;
        totalTime += dt;
        // For debugging and tuning
        displayString(1, "%.2f", error);
        displayString(2, "%.2f", u);
        displayString(3, "%.2f", nMotorEncoderRaw[motor0]);
    }
    motor[motor0] = 0;
}

// Rotate a motor without PID
void rotate(int motor0, int degrees, int power, bool CW) {
    int dir = 1;
    if (!CW) {
        dir *= -1;
    }
    nMotorPIDSpeedCtrl[motor0] = mtrSpeedReg;
    nMotorEncoder[motor0] = 0;
    motor[motor0] = power * dir;
    while (abs(nMotorEncoder[motor0]) < abs(degrees)) {}
    motor[motor0] = 0;
}

/*
 * PID constants are empirically determined
 * for each movement needed, so there is a
 * function for each movement.
 *
 * PID does not only need to guarantee
 * precise motor movement, to avoid error
 * build-up, but it must also ensure that,
 * the cube is in the correct position
 * after executing that move. K_p is tuned
 * first, then K_d, and finally K_i. A little
 * bit of oscillation is desirable to "wiggle"
 * the cube into its final position.
 */
void rotateBase90(bool CW) {
    rotatePID(BASE_MOTOR, 90, 800,
              5, 0, 0.012,
              CW, 10);
}
void rotateBase180() {
    rotatePID(BASE_MOTOR, 180, 1000,
              5, 0, 0.009,
              true, 10);
}

// Clamp and unclamp to rotate bottom face
void clampCube() {
    rotate(CLAMP_MOTOR, 40, 30, true);
    while(motorsRunning()) {}
    wait1Msec(250);
}
void unclampCube() {
    rotate(CLAMP_MOTOR, 40, 30, false);
    while(motorsRunning()) {}
    wait1Msec(250);
}

// Rotate in the y-plane clockwise
void flipCube() {
    // physical stops means we do not need PID
    rotate(CLAMP_MOTOR, 65, 65, true);
    while(motorsRunning()) {}
    wait1Msec(600);
    rotate(CLAMP_MOTOR, 65, 40, false);
    while(motorsRunning()) {}
    wait1Msec(250);
}

// Change the faces array to reflect a movement
const int UP = 0, FRONT = 1, DOWN = 2, BACK = 3, RIGHT = 4, LEFT = 5;
/* U    F    D    B    R    L */
char faces[6] = {'U', 'F', 'D', 'B', 'R', 'L'};

void applyRotation_x(bool CW) {
    // U and D unaffected
    // F, R, B, L shifted
    if (CW) {
        char temp = faces[FRONT]; // there's a better way but I'm tired
        faces[FRONT] = faces[RIGHT];
        faces[RIGHT] = faces[BACK];
        faces[BACK] = faces[LEFT];
        faces[LEFT] = temp;
    } else {
        char temp = faces[FRONT];
        faces[FRONT] = faces[LEFT];
        faces[LEFT] = faces[BACK];
        faces[BACK] = faces[RIGHT];
        faces[RIGHT] = temp;
    }
}
void applyRotation_y() {
    // Always clockwise
    // L and R unaffected
    // U, R, D, L shifted
    char temp = faces[UP];
    faces[UP] = faces[FRONT];
    faces[FRONT] = faces[DOWN];
    faces[DOWN] = faces[BACK];
    faces[BACK] = temp;
}

// Turn the cube such that a face is against the base
int getRotY(char face) {
    // will never be called if faces[DOWN] == face
    int rot_y = -1;
    if (faces[BACK] == face) {
        rot_y = 1;
    } else if (faces[UP] == face) {
        rot_y = 2;
    } else if (faces[FRONT] == face) {
        rot_y = 3;
    }
    return rot_y;
}
void rotateToFace(char face) {
    // Down is against the base
    if (faces[DOWN] == face) {
        return;
    }
    int rot_y = getRotY(face);
    if (rot_y < 0) {
        // now we can minimize y-rotations
        if (faces[LEFT] == face) {
            rotateBase90(true);
            applyRotation_x(true);
        } else {
            rotateBase90(false);
            applyRotation_x(false);
        }
        rot_y = getRotY(face);
    }
    if (rot_y > 0) {
        if (rot_y == 3) {
            rotateBase180();
            clampCube();
            unclampCube();
            flipCube();
            applyRotation_x(true);
            applyRotation_x(true);
            applyRotation_y();
        } else {
            for (int rot = 0; rot < rot_y; rot++) {
                if (rot == 0) {
                    clampCube();
                    unclampCube();
                }
                flipCube();
                applyRotation_y();
                if (rot != (rot_y - 1)) {
                    clampCube();
                    unclampCube();
                }
            }
        }
    }
}
void doMove(char face, int rot) {
    displayString(0, "%c%d", face, rot);
    rotateToFace(face);
    clampCube();
    if (rot == 2) {
        rotateBase180();
    } else {
        rotateBase90(rot == 3);
    }
    unclampCube();
}

char moveArr[500]; // must be global due to local size limit
void doMoves(string moves) {
    strcpy(moveArr, moves);
    for (int i = 0; i < 50; i+=2) {
        if (moveArr[i] == (char)0) {
            break;
        }
        doMove(moveArr[i], (int)moveArr[i+1] - 48);
    }

}

const int size = 7;
string moves[size] = { // due to the local array size limit
        "L1D2B1L3F1",
        "L2D2L3U2R1",
        "D1U2L1R2D3",
        "L2D3L2D1B2",
        "D3B2U1L2D2",
        "L2B2R2D2F2",
        "D2B2R2D2"
};
task main() {
    for (int i = 0; i < size; i++) {
        doMoves(moves[i]);
    }
}