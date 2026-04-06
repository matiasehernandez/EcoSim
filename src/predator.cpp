/**
 * predator.cpp
 * Implementación de la clase Predator
 */

#include "ecosim.h"

Predator::Predator(double x, double y)
    : Agent(x, y, PRED_COLOR, PRED_HEAD_SIZE, PRED_SPEED,
            PRED_VISION_RADIUS, PRED_VISION_ANGLE, PRED_MOVE_COST) {}

void Predator::update(std::vector<Prey>& prey, double pred_vr, double pred_va,
                      double pred_mc, double p_catch) {
    if (!alive) return;

    vision_r = pred_vr;
    vision_a_deg = pred_va;
    move_cost   = pred_mc;

    // Cazar presas
    Prey* chase_target = nullptr;
    double min_d = vision_r;
    for (auto& p : prey) {
        if (!p.alive) continue;
        if (can_see(p.x, p.y)) {
            double d = dist(x, y, p.x, p.y);
            if (d < min_d) {
                min_d = d;
                chase_target = &p;
            }
        }
    }

    if (chase_target) {
        step(angle_towards(x, y, chase_target->x, chase_target->y));
        if (dist(x, y, chase_target->x, chase_target->y) < (PRED_HEAD_SIZE/2 + PREY_HEAD_SIZE/2 + 2)) {
            if (random_double(0, 1) < p_catch) {
                chase_target->alive = false;
                energy = std::min(energy + PRED_ENERGY_GAIN, 250.0);
            }
        }
    } else {
        step();
    }
}

Predator* Predator::try_reproduce() {
    if (energy > PRED_REPRO_THRESHOLD && random_double(0, 1) < PRED_REPRO_PROB) {
        energy *= 0.55;
        double nx = x + random_double(-20, 20);
        double ny = y + random_double(-20, 20);
        wrap(nx, ny);
        return new Predator(nx, ny);
    }
    return nullptr;
}
