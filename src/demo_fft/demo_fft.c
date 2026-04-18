#include <float.h>
#include <arm_math.h> // CMSIS-DSP Header
#include "utils.h"
#include "math.h"

#define SAMPLES 1024
#define SAMPLING_FREQ 1000.0f
#define TARGET_FREQ 50.0f

float input_signal[SAMPLES];

void simulate_sensor_data()
{
    for (int i = 0; i < SAMPLES; i++)
    {
        float t = (float)i / SAMPLING_FREQ;
        input_signal[i] = sinf(2.0f * 3.14 * TARGET_FREQ * t);
    }
}

int main()
{
    print_string("Simulating sensor data...\n");
    simulate_sensor_data();
    print_string("Input signal, every 20th sample:\n");

    for (int i = 0; i < SAMPLES / 2; i += 20)
    {
        float real = input_signal[i];
        char buffer[20];
        num_to_string(real, buffer, 4);
        print_string(buffer);
    }

    float32_t test_output[SAMPLES];
    arm_rfft_fast_instance_f32 fft_instance;
    arm_rfft_fast_init_1024_f32(&fft_instance);

    arm_rfft_fast_f32(&fft_instance, input_signal, test_output, 0);

    print_string("\nFFT output, every 20th sample:\n");
    for (int i = 0; i < SAMPLES / 2; i += 20)
    {
        float real = test_output[i];
        char buffer[20];
        num_to_string(real, buffer, 4);
        print_string(buffer);
    }

    while (1)
    {
    }
}