typedef struct Joystick_Msg
{
    float x;
    float y;
    int a;
    int b;
    long timestamp;
} Joystick_Msg;

typedef struct Power_Msg
{
    float voltage;
    float current;
} Power_Msg;
 
typedef struct Position_Msg
{
    float lat; // decimal 
    float lon; // decimal
    float alt; // meters
    float acc; // horizontal accuracy in millimeters
    int   rtk; // 0: no RTK solution, 1: float solution, 2: fixed solution
} Position_Msg;
