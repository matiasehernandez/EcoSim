/**
 * prey.cpp
 * Implementación de la clase Prey
 */

#include "ecosim.h"

Prey::Prey(double x, double y)
    : Agent(x, y, PREY_COLOR, PREY_HEAD_SIZE, PREY_SPEED,
            PREY_VISION_RADIUS, PREY_VISION_ANGLE, PREY_MOVE_COST) {}

void Prey::update(std::vector<Grass>& grass, std::vector<Predator>& predators,
                  double prey_vr, double prey_va, double prey_mc, int grass_regrow) {
    if (!alive) return;

    vision_r = prey_vr;
    vision_a_deg = prey_va;
    move_cost   = prey_mc;

    // Huir de depredadores
    Predator* flee_target = nullptr;
    double min_d = vision_r;
    for (auto& pred : predators) {
        if (!pred.alive) continue;
        if (can_see(pred.x, pred.y)) {
            double d = dist(x, y, pred.x, pred.y);
            if (d < min_d) {
                min_d = d;
                flee_target = &pred;
            }
        }
    }

    if (flee_target) {
        step(angle_away(x, y, flee_target->x, flee_target->y));
        return;
    }

    // Buscar comida
    Grass* food_target = nullptr;
    min_d = vision_r;
    for (auto& g : grass) {
        if (!g.alive) continue;
        if (can_see(g.x, g.y)) {
            double d = dist(x, y, g.x, g.y);
            if (d < min_d) {
                min_d = d;
                food_target = &g;
            }
        }
    }

    if (food_target) {
        step(angle_towards(x, y, food_target->x, food_target->y));
        if (dist(x, y, food_target->x, food_target->y) < (PREY_HEAD_SIZE/2 + GRASS_SIZE/2 + 2)) {
            food_target->eat(grass_regrow);
            energy = std::min(energy + PREY_ENERGY_GAIN, 200.0);
        }
    } else {
        step();
    }
}

Prey* Prey::try_reproduce() {
    if (energy > PREY_REPRO_THRESHOLD && random_double(0, 1) < PREY_REPRO_PROB) {
        energy *= 0.6;
        double nx = x + random_double(-20, 20);
        double ny = y + random_double(-20, 20);
        wrap(nx, ny);
        return new Prey(nx, ny);
    }
    return nullptr;
}
