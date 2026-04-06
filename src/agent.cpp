/**
 * agent.cpp
 * Implementación de la clase base Agent
 */

#include "ecosim.h"

Agent::Agent(double x, double y, Fl_Color color, int head_size, double speed,
             double vision_r, double vision_a_deg, double move_cost)
    : x(x), y(y), color(color), head_size(head_size), speed(speed),
      vision_r(vision_r), vision_a_deg(vision_a_deg), move_cost(move_cost),
      energy(INIT_ENERGY), alive(true), angle(random_angle()), accum(0.0) {
    for (int i = 0; i < N_BODY_SEGS; i++) {
        body.push_back({x, y});
    }
}

bool Agent::can_see(double tx, double ty) {
    if (dist(x, y, tx, ty) > vision_r) return false;
    double half = (vision_a_deg / 2.0) * M_PI / 180.0;
    return in_cone(angle, x, y, tx, ty, half);
}

void Agent::step(double new_angle) {
    if (new_angle != -999.0) {
        angle = new_angle;
    } else if (random_double(0, 1) < P_TURN) {
        angle += random_double(-0.6, 0.6);
    }

    x += std::cos(angle) * speed;
    y += std::sin(angle) * speed;
    wrap(x, y);
    energy -= move_cost;

    if (energy <= 0) alive = false;

    accum += speed;
    if (accum >= SEG_GAP) {
        body.push_front({x, y});
        if ((int)body.size() > N_BODY_SEGS) body.pop_back();
        accum = 0.0;
    }
}

void Agent::draw(int cx, int cy) {
    if (!alive) return;

    // Dibujar segmentos del cuerpo (orden decreciente: cabeza más grande)
    std::vector<std::pair<double, double>> segments(body.begin(), body.end());

    for (size_t i = 0; i < segments.size(); i++) {
        double sx = segments[i].first;
        double sy = segments[i].second;
        double frac = (double)i / (N_BODY_SEGS);
        int s = std::max(3, (int)(head_size * (1.0 - 0.65 * frac)));

        unsigned char r = (unsigned char)(fl_color_red(color) * (1.0 - 0.5 * frac));
        unsigned char g = (unsigned char)(fl_color_green(color) * (1.0 - 0.5 * frac));
        unsigned char b = (unsigned char)(fl_color_blue(color)  * (1.0 - 0.5 * frac));
        fl_color(fl_rgb_color(std::max(30, (int)r), std::max(30, (int)g), std::max(30, (int)b)));
        fl_rectf((int)sx - s/2, (int)sy - s/2, s, s);
    }

    // Dibujar cabeza (más brillante)
    fl_color(color);
    fl_rectf(cx - head_size/2, cy - head_size/2, head_size, head_size);
    int inner = std::max(2, head_size - 4);
    int r = std::min(255, fl_color_red(color) + 70);
    int g = std::min(255, fl_color_green(color) + 70);
    int b = std::min(255, fl_color_blue(color)  + 70);
    fl_color(fl_rgb_color(r, g, b));
    fl_rectf(cx - inner/2, cy - inner/2, inner, inner);
}

void Agent::draw_vision(int cx, int cy, bool show) {
    if (!show || !alive) return;

    int r = (int)vision_r;
    double half = (vision_a_deg / 2.0) * M_PI / 180.0;

    // Sector de visión
    fl_color(fl_rgb_color(fl_color_red(color)/3,
                          fl_color_green(color)/3,
                          fl_color_blue(color)/3));
    fl_begin_polygon();
    fl_vertex(cx, cy);
    for (int i = 0; i <= 30; i++) {
        double a = angle - half + (2 * half) * (i / 30.0);
        fl_vertex(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    fl_end_polygon();

    // Líneas del cono
    fl_color(color);
    fl_line(cx, cy, cx + r * std::cos(angle - half), cy + r * std::sin(angle - half));
    fl_line(cx, cy, cx + r * std::cos(angle + half), cy + r * std::sin(angle + half));

    // Arco
    fl_arc(cx - r, cy - r, 2 * r, 2 * r,
           -(angle + half) * 180 / M_PI, -(angle - half) * 180 / M_PI);
}
