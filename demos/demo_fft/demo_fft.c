/*
 * @filename: demo_fft.c
 * @author:   Lukáš Čačík
 * @date:     May 09, 26
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <float.h>
#include <arm_math.h>
#include "utils.h"
#include "math.h"

#define SAMPLES 1024
#define SAMPLING_FREQ 1000.0f
#define TARGET_FREQ 50.0f
volatile uint32_t benchmark_result = 0;

float input_signal[SAMPLES];

void simulate_sensor_data()
{

    for (int i = 0; i < SAMPLES; i++)
    {

        float t = (float)i / SAMPLING_FREQ;
        input_signal[i] = sinf(2.0f * 3.14 * TARGET_FREQ * t);

    }

}
float32_t test_output[SAMPLES];

int main()
{


    simulate_sensor_data();

    reset_cycle_counter();
    uint32_t start = get_cycles();

    arm_rfft_fast_instance_f32 fft_instance;

    arm_rfft_fast_init_1024_f32(&fft_instance);

    arm_rfft_fast_f32(&fft_instance, input_signal, test_output, 0);

    uint32_t end = get_cycles();

    benchmark_result = end - start;

    while (1)
    {
    }
}
