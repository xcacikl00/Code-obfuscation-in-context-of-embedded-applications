
#include <utils.h>

// 1. Struct representing the state of the controller
typedef struct
{
    float Kp;
    float Ki;
    float Kd;

    float setpoint;
    float integral;
    float prev_error;

    //  output limits
    float out_min;
    float out_max;
} PID_Controller;

float simulate_sensor(float current_temp, float heater_power)
{

    float cooling = 0.1f;
    float heating = heater_power * 0.01f;

    return current_temp + heating - cooling;
}

float pid_compute(PID_Controller *pid, float measured_value)
{
    float error = pid->setpoint - measured_value;

    float p_out = pid->Kp * error;

    pid->integral += error;

    // anti - windup
    if (pid->integral > 50.0f)
        pid->integral = 50.0f;
    if (pid->integral < -50.0f)
        pid->integral = -50.0f;
        
    float i_out = pid->Ki * pid->integral;

    float derivative = error - pid->prev_error;
    float d_out = pid->Kd * derivative;

    float output = p_out + i_out + d_out;

    if (output > pid->out_max)
        output = pid->out_max;
    else if (output < pid->out_min)
        output = pid->out_min;

    pid->prev_error = error;

    return output;
}

int main(void)
{
    PID_Controller controller = {
        .Kp = 3.5f,
        .Ki = 0.2f,
        .Kd = 1.5f,
        .setpoint = 75.0f, //  target
        .integral = 0.0f,
        .prev_error = 0.0f,
        .out_min = 0.0f,
        .out_max = 100.0f};

    float current_sensor_value = 65.0f; // starting temperature
    float control_signal = 0.0f;

    // adjustment loop
    for (int i = 0; i < 50; i++)
    {
        current_sensor_value = simulate_sensor(current_sensor_value, control_signal);
        control_signal = pid_compute(&controller, current_sensor_value);

        print_string("step ");
        char s_buf[20];

        num_to_string(i, s_buf, 2);
        print_string(s_buf);

        print_string(" sensor: ");
        num_to_string(current_sensor_value, s_buf, 2);
        print_string(s_buf);

        print_string(" output: ");
        num_to_string(control_signal, s_buf, 2);
        print_string(s_buf);

        print_string("\n");
    }

    return 0;
}