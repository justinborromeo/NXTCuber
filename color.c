#include "NXT_FileIO.c"

int getColor(float H, float S, float L) {
    int X[3] = { H, S, L };
    if (X[1] <= 7.3307) {
        if (X[0] > 230) {
            return 3;
        }
        else {
            return 4;
        }
    }
    else {
        if (X[0] <= 8.6519) {
            return 2;
        }
        else {
            if (X[2] > 14.3379) {
                if (X[0] <= 174.6618) {
                    return 0;
                }
                else {
                    return 1;
                }
            }
            else {
                if (X[1] > 13.5021) {
                    return 5;
                }
                else {
                    return 4;
                }
            }
        }
    }
}

float min(float a, float b) {
    if (a > b) {
        return b;
    } else {
        return a;
    }
}

float max(float a, float b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

void read(float &hue, float &sat, float &lum) {
    int red, green, blue;
    SensorType[S1]=sensorColorNxtRED;
    clearTimer(T1);
    while(time1[T1]<200) {
        red=SensorRaw[S1];
    }

    SensorType[S1]=sensorColorNxtGREEN;
    clearTimer(T1);
    while(time1[T1]<200) {
        green=SensorRaw[S1];
    }
    SensorType[S1]=sensorColorNxtBLUE;
    clearTimer(T1);
    while(time1[T1]<200) {
        blue=SensorRaw[S1];
    }
    displayString(0, "R: %3d", red);
    displayString(1, "G: %3d", green);
    displayString(2, "B: %3d", blue);
    float R = red / 1027.0;
    float G = green / 1027.0;
    float B = blue / 1027.0;
    float Max = max(R, max(G, B));
    float Min = min(R, min(G, B));
    float L = (Max + Min) * 50.0;
    float S, H;
    if (Max == Min) {
        S = 0;
        H = 0;
    } else {
        if (L <= 50) {
            S = (Max - Min) / (Max + Min);
        } else {
            S = (Max - Min) / (2.0 - Max - Min);
        }
        S = 100 * S;
        if (fabs(Max - R) <= 0.0001) {
            H = (G - B) / (Max - Min);
        } else if (fabs(Max - G) <= 0.0001) {
            H = 2.0 + (B - R) / (Max - Min);
        } else {
            H = 4.0 + (R - G) / (Max - Min);
        }
    }
    H = H * 60;
    if (H < 0) {
        H = H + 360;
    }
    displayString(3, "H: %5.2f", H);
    displayString(4, "S: %5.2f", S);
    displayString(5, "L: %5.2f", L);

    hue = H;
    sat = S;
    lum = L;
}

char *map = "WYROBG";

task main() {
    /*
  float H, S, L;
  for(;;){
  read(H, S, L);
  int color = getColor(H, S, L);
  displayString(6, "Color: %c", map[color]);
  while(nNxtButtonPressed == -1) {}
    while(nNxtButtonPressed != -1) {}}
    */
    TFileHandle fout;
    word fileSize = 10000;


    float H, S, L;
    for(int k = 0; k < 6; k++) {
        string name = "color_";
        strcat(name, &map[k]);
        strcat(name, ".txt");
        bool fileOkay = openWritePC(fout, name, fileSize);
        for(int j = 0; j < 3; j++) {
            while(nNxtButtonPressed != 3) {
                read(H, S, L);
            }
            for (int g = 1; g <= 100; g++) {
                string b;
                switch(j) {
                    case 0: b = "Mid"; break;
                    case 1: b = "Edge"; break;
                    case 2: b = "Corn"; break;
                    default: b = "NaN"; break;
                }
                displayString(6, "%c %4s %d", map[k], b, g);
                read(H, S, L);
                writeFloatPC(fout, "%.2f", H);
                writeCharPC(fout, ',');
                writeFloatPC(fout, "%.2f", S);
                writeCharPC(fout, ',');
                writeFloatPC(fout, "%.2f", L);
                writeCharPC(fout, ',');
            }
            while(nNxtButtonPressed == -1) {}
            while(nNxtButtonPressed != -1) {}
        }
        closeFilePC(fout);
        displayString(6, "        ");
    }

}

//W, Y, R, O, B, G