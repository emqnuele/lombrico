#ifndef LOMBRICO_HUD_H
#define LOMBRICO_HUD_H

#include "canvas.h"
#include "ear.h"
#include "food.h"
#include "soil.h"

void hud_draw(struct canvas *, const struct soil *, const struct hearing *,
    const struct food *);

#endif
