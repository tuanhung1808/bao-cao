#include "PID.h"
#include "SYSTICK.h"
#include "motor_config.h"
PID_Controller Motor_PID = {
    .Kp = MOTOR_KP, .Ki = MOTOR_KI, .Kd = MOTOR_KD,
    .Out_Max = 35999.0f, .Out_Min = 0.0f,
    .FeedforwardGain = MOTOR_FEEDFORWARD_RPM > 0.0f ? 35999.0f / MOTOR_FEEDFORWARD_RPM : 0.0f
};
static float Clamp(float x, float lo, float hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}
void PID_Reset(PID_Controller *pid)
{
    pid->Integral = pid->Prev_Error = pid->Prev_Measurement = pid->Last_Output = 0.0f;
    pid->Last_Time = millis();
    pid->Initialized = 0;
}
float PID_Compute(PID_Controller *pid, float speed)
{
    uint32_t now = millis();
    if (pid->Setpoint <= 0.0f) { PID_Reset(pid); return 0.0f; }
    uint32_t elapsed = now - pid->Last_Time;
    float dt, slope = 0.0f;
    if (!pid->Initialized) {
        dt = CONTROL_PERIOD_MS * 0.001f;
        pid->Initialized = 1;
    } else {
        if (!elapsed) return pid->Last_Output;
        dt = elapsed * 0.001f;
        slope = (speed - pid->Prev_Measurement) / dt;
    }
    float error = pid->Setpoint - speed;
    float p = pid->Kp * error, d = -pid->Kd * slope;
    float old_i = pid->Ki > 0.0f ? pid->Integral : 0.0f;
    float next_i = pid->Ki > 0.0f ? Clamp(old_i + error * dt, -pid->Out_Max / pid->Ki, pid->Out_Max / pid->Ki) : 0.0f;
    float ff = pid->FeedforwardGain * pid->Setpoint;
    float trial = ff + p + pid->Ki * next_i + d;
    /* Conditional integration: do not wind up against a saturated output. */
    if ((trial > pid->Out_Max && error > 0.0f) || (trial < pid->Out_Min && error < 0.0f))
        next_i = old_i;
    pid->Integral = next_i;
    pid->Prev_Error = error;
    pid->Prev_Measurement = speed;
    pid->Last_Time = now;
    pid->Last_Output = Clamp(ff + p + pid->Ki * next_i + d, pid->Out_Min, pid->Out_Max);
    return pid->Last_Output;
}
