#ifndef MOTOR_PID_H
#define MOTOR_PID_H
#include <stdint.h>
typedef struct {
    float Kp, Ki, Kd, Setpoint, Integral, Prev_Error;
    uint32_t Last_Time;
    float Out_Max, Out_Min, Prev_Measurement, Last_Output, FeedforwardGain;
    uint8_t Initialized;
} PID_Controller;
extern PID_Controller Motor_PID;
void PID_Reset(PID_Controller *pid);
float PID_Compute(PID_Controller *pid, float current_speed);
#endif
