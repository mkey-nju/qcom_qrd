#ifndef TOUCH_H
#define TOUCH_H
typedef struct touch
{
    int x;
    int y;
    int pressed;
} touch_t;
extern void set_touch_pos(touch_t *t);
#endif
