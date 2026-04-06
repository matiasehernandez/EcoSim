/**
 * grass.cpp
 * Implementación de la clase Grass
 */

#include "ecosim.h"

Grass::Grass(double x, double y) : x(x), y(y), alive(true), regrow_timer(0) {}

void Grass::update(int /*regrow_time*/) {
    if (!alive) {
        regrow_timer--;
        if (regrow_timer <= 0) {
            alive = true;
        }
    }
}

void Grass::eat(int regrow_time) {
    alive = false;
    regrow_timer = regrow_time;
}

void Grass::draw(int cx, int cy) {
    if (!alive) return;
    int s = GRASS_SIZE;
    fl_color(GRASS_COLOR);
    fl_rectf(cx - s/2, cy - s/2, s, s);
    int inner = std::max(2, s - 4);
    fl_color(fl_rgb_color(120, 240, 140));
    fl_rectf(cx - inner/2, cy - inner/2, inner, inner);
}
