#ifndef __DSP_FILTERS_H
#define __DSP_FILTERS_H

typedef struct
{
    float lpf_prev;
    float hpf_prev;
    float hpf_input_prev;
} FilterState_t;

void Filter_Init(FilterState_t *f);
float Filter_HighPass(FilterState_t *f, float input);
float Filter_LowPass(FilterState_t *f, float input);
float Filter_BandPass(FilterState_t *f, float input);

#endif
