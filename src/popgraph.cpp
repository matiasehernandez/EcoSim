/**
 * popgraph.cpp
 * Implementación de la clase PopGraph
 */

#include "ecosim.h"

PopGraph::PopGraph() {
    for (int i = 0; i < HISTORY_LEN; i++) {
        pred_hist.push_back(0);
        prey_hist.push_back(0);
        grass_hist.push_back(0);
    }
}

void PopGraph::push(int np, int ny, int ng) {
    pred_hist.push_back(np);
    prey_hist.push_back(ny);
    grass_hist.push_back(ng);
    pred_hist.pop_front();
    prey_hist.pop_front();
    grass_hist.pop_front();

    pred_full.push_back(np);
    prey_full.push_back(ny);
    grass_full.push_back(ng);
}

void PopGraph::draw(int x, int y, int w, int h) {
    fl_color(fl_rgb_color(8, 10, 22));
    fl_rectf(x, y, w, h);
    fl_color(GRID_COLOR);
    fl_rect(x, y, w, h);

    auto draw_line = [&](std::deque<int>& hist, Fl_Color col) {
        int max_val = 1;
        for (int v : hist) {
            if (v > max_val) max_val = v;
        }
        if (max_val < 1) max_val = 1;

        std::vector<std::pair<int, int>> points;
        int n = hist.size();
        for (size_t i = 0; i < hist.size(); i++) {
            int px = x + (int)((double)i / n * w);
            int py = y + h - 2 - (int)((double)hist[i] / max_val * (h - 4));
            points.push_back({px, py});
        }

        fl_color(col);
        for (size_t i = 1; i < points.size(); i++) {
            fl_line(points[i-1].first, points[i-1].second,
                    points[i].first, points[i].second);
        }
    };

    draw_line(grass_hist, GRASS_COLOR);
    draw_line(prey_hist, PREY_COLOR);
    draw_line(pred_hist, PRED_COLOR);

    fl_color(WHITE);
    fl_font(FL_HELVETICA, 10);
    fl_draw("Dep", x +  4, y + 12);
    fl_draw("Pre", x + 30, y + 12);
    fl_draw("Hie", x + 56, y + 12);
}
