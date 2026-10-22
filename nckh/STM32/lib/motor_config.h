#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H
/* Actual TIM2 counts/rev of measured shaft. Original value, NOT verified
 * for the Proteus motor! Encoder mode 3 counts x4 per channel-A cycle.
 * Include gearbox ratio only for an encoder upstream of the measured shaft.
 */
#define ENCODER_COUNTS_PER_REV 937.2f
#define MOTOR_MAX_RPM          280u
#define CONTROL_PERIOD_MS      50u
#define TELEMETRY_PERIOD_MS    200u
#define COMMAND_TIMEOUT_MS    500u
#define PWM_FREQUENCY_HZ       2000u
#define SPEED_FILTER_TAU_S     0.10f
#define REVERSE_COAST_MS       200u
#define REVERSE_STOP_RPM       5.0f
#define REVERSE_STABLE_SAMPLES 3u
/* Nominal no-load RPM at full PWM, starting assumption from the original
 * 280-RPM motor. Set to measured no-load speed after encoder calibration.
 * Set to 0.0f to disable feedforward and use feedback PID only.
 */
#define MOTOR_FEEDFORWARD_RPM 280.0f
/* Original PID gains retained as a starting point, not plant-validated. */
#define MOTOR_KP 20.0f
#define MOTOR_KI 8.0f
#define MOTOR_KD 0.05f
#endif
