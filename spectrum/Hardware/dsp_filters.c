#include "dsp_filters.h"

void Filter_Init(FilterState_t *f)
{
    if (f == 0) return;

    f->lpf_prev = 0.0f;
    f->hpf_prev = 0.0f;
    f->hpf_input_prev = 0.0f;
}

float Filter_HighPass(FilterState_t *f, float input)
{
    const float alpha = 0.995f;
    float output;

    if (f == 0) return input;

    output = alpha * (f->hpf_prev + input - f->hpf_input_prev);
    f->hpf_prev = output;
    f->hpf_input_prev = input;

    return output;
}

float Filter_LowPass(FilterState_t *f, float input)
{
    const float alpha = 0.75f;

    if (f == 0) return input;

    f->lpf_prev = alpha * f->lpf_prev + (1.0f - alpha) * input;
    return f->lpf_prev;
}

float Filter_BandPass(FilterState_t *f, float input)
{
    float y;
    y = Filter_HighPass(f, input);
    y = Filter_LowPass(f, y);
    return y;
}
