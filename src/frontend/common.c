#include "common.h"

FitRect fit_rect_to_ratio(float cx, float cy, float cw, float ch, float ratio)
{
    double c_ratio = cw / ch;

    if (c_ratio > ratio) {
        float w = ch * ratio;
        return (FitRect){
            .x = cx + ((cw - w) / 2.0F),
            .y = cy,
            .w = w,
            .h = ch,
        };
    }

    if (c_ratio < ratio) {
        float h = cw / ratio;
        return (FitRect){
            .x = cx,
            .y = cy + ((ch - h) / 2.0F),
            .w = cw,
            .h = h,
        };
    }

    return (FitRect){.x = cx, .y = cy, .w = cw, .h = ch};
}
