typedef struct joystick_msg
{
    float x;
    float y;
    int a;
    int b;
    long timestamp;
} joystick_msg;

typedef struct power_msg
{
    float voltage;
    float current;
} power_msg;
 
typedef struct position_msg
{
    float lat; // decimal 
    float lon; // decimal
    float alt; // meters
    float acc; // horizontal accuracy in millimeters
    int   rtk; // 0: no RTK solution, 1: float solution, 2: fixed solution
} position_msg;
