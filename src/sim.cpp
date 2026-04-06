/**
 * sim.cpp
 * Implementación de la clase Sim
 *
 * Controles:
 *   [R]   Reiniciar
 *   [P]   Pausa / reanudar
 *   [V]   Mostrar/ocultar conos de visión
 *   [S]   Guardar
 *   [Esc] Salir
 */

#include "ecosim.h"
#include <sys/stat.h>

// ===========================================================================
// Constructor / Destructor
// ===========================================================================

Sim::Sim() : Fl_Double_Window(WIDTH + PANEL_W, HEIGHT, "EcoSim - Modelo basado en agentes"),
    p_catch(P_CATCH), pred_vision_radius(PRED_VISION_RADIUS),
    pred_vision_angle(PRED_VISION_ANGLE), prey_vision_radius(PREY_VISION_RADIUS),
    prey_vision_angle(PREY_VISION_ANGLE), grass_regrow_time(GRASS_REGROW_TIME),
    pred_move_cost(PRED_MOVE_COST), prey_move_cost(PREY_MOVE_COST),
    paused(false), show_vision(false), step(0), save_timer(0) {

    resizable(this);

    // Crear sliders - cada bloque ocupa 45px (label 14px + slider 22px + gap 9px)
    int sx = WIDTH + 10;
    int sw = PANEL_W - 20;

    struct SliderDef {
        const char* label;
        double lo, hi, val;
    } defs[8] = {
        {"p_captura [0.05-1]", 0.05, 1.0, p_catch},
        {"radio dep. [40-250]", 40, 250, pred_vision_radius},
        {"angulo dep. [10-360]", 10, 360, pred_vision_angle},
        {"radio presa [30-200]", 30, 200, prey_vision_radius},
        {"angulo presa [10-360]", 10, 360, prey_vision_angle},
        {"regrowth [50-400]", 50, 400, (double)grass_regrow_time},
        {"costo dep. [0.05-0.8]", 0.05, 0.8, pred_move_cost},
        {"costo presa [0.05-0.6]", 0.05, 0.6, prey_move_cost},
    };

    int base_y = 105;
    for (int i = 0; i < 8; i++) {
        int sy = base_y + i * 45;
        Fl_Slider* s = new Fl_Slider(sx, sy + 15, sw, 20);
        s->type(FL_HOR_SLIDER);
        s->label("");
        s->labelsize(11);
        s->labelcolor(FL_BLACK);
        s->labelfont(FL_HELVETICA_BOLD);
        s->color(fl_rgb_color(60, 65, 110));
        s->selection_color(fl_rgb_color(80, 140, 220));
        s->bounds(defs[i].lo, defs[i].hi);
        s->value(defs[i].val);
        s->step((defs[i].hi - defs[i].lo) / 200.0);
        s->callback(slider_cb, this);
        sliders.push_back(s);
    }

    pred_count = nullptr; prey_count = nullptr;
    grass_count = nullptr; step_count = nullptr;

    // Botones - debajo del último slider 
    int btn_y = 470;
    btn_reset  = new Fl_Button(WIDTH + 10, btn_y, 115, 26, "[R] Reiniciar");
    btn_pause  = new Fl_Button(WIDTH + 135, btn_y, 115, 26, "[P] Pausa");
    btn_vision = new Fl_Button(WIDTH + 10, btn_y + 32, 115, 26, "[V] Vision");
    btn_save   = new Fl_Button(WIDTH + 135, btn_y + 32, 115, 26, "[S] Guardar");

    btn_reset->callback(reset_cb, this);
    btn_pause->callback(pause_cb, this);
    btn_vision->callback(vision_cb, this);
    btn_save->callback(save_cb, this);

    reset();
    Fl::add_timeout(1.0/FPS, timer_cb, this);
    end();
    show();
}

Sim::~Sim() {
    Fl::remove_timeout(timer_cb, this);
}

// ===========================================================================
// Callbacks estáticos (mejorar)
// ===========================================================================

void Sim::timer_cb(void* data) {
    Sim* sim = (Sim*)data;
    if (!sim->paused) sim->update();
    sim->redraw();
    Fl::repeat_timeout(1.0/FPS, timer_cb, data);
}

void Sim::reset_cb(Fl_Widget*, void* data) {
    ((Sim*)data)->reset();
}

void Sim::pause_cb(Fl_Widget*, void* data) {
    Sim* sim = (Sim*)data;
    sim->paused = !sim->paused;
    sim->btn_pause->label(sim->paused ? "[P] Reanudar" : "[P] Pausa");
}

void Sim::vision_cb(Fl_Widget*, void* data) {
    Sim* sim = (Sim*)data;
    sim->show_vision = !sim->show_vision;
    sim->btn_vision->label(sim->show_vision ? "[V] Ocultar" : "[V] Vision");
}

void Sim::save_cb(Fl_Widget*, void* data) {
    ((Sim*)data)->do_save();
}

void Sim::slider_cb(Fl_Widget* w, void* data) {
    Sim* sim = (Sim*)data;
    Fl_Slider* s = (Fl_Slider*)w;

    // Identificar por posición en el vector (mismo orden que defs[])
    int idx = -1;
    for (int i = 0; i < (int)sim->sliders.size(); i++) {
        if (sim->sliders[i] == s) { idx = i; break; }
    }

    switch (idx) {
        case 0: sim->set_p_catch(s->value()); break;
        case 1: sim->set_pred_vision_radius(s->value()); break;
        case 2: sim->set_pred_vision_angle(s->value()); break;
        case 3: sim->set_prey_vision_radius(s->value()); break;
        case 4: sim->set_prey_vision_angle(s->value()); break;
        case 5: sim->set_grass_regrow_time((int)s->value()); break;
        case 6: sim->set_pred_move_cost(s->value()); break;
        case 7: sim->set_prey_move_cost(s->value()); break;
    }
}

// ===========================================================================
// Lógica de simulación
// ===========================================================================

void Sim::update_ui() {
    (void)pred_count; (void)prey_count; (void)grass_count; (void)step_count;
}

void Sim::reset() {
    reseed_rng();
    grass.clear();
    prey.clear();
    predators.clear();

    for (int i = 0; i < N_GRASS; i++) grass.push_back(Grass(random_double(0, WIDTH), random_double(0, HEIGHT)));
    for (int i = 0; i < N_PREY; i++) prey.push_back(Prey(random_double(0, WIDTH), random_double(0, HEIGHT)));
    for (int i = 0; i < N_PREDATORS; i++) predators.push_back(Predator(random_double(0, WIDTH), random_double(0, HEIGHT)));

    step = 0;
    graph = PopGraph();
    update_ui();
}

void Sim::update() {
    // Hierba
    for (auto& g : grass) g.update(grass_regrow_time);

    // Presas
    std::vector<Prey> new_prey;
    for (auto& p : prey) {
        if (p.alive) p.update(grass, predators, prey_vision_radius, prey_vision_angle, prey_move_cost, grass_regrow_time);
        if (p.alive) { Prey* child = p.try_reproduce(); if (child) new_prey.push_back(*child); }
    }
    prey.erase(std::remove_if(prey.begin(), prey.end(), [](const Prey& p){ return !p.alive; }), prey.end());
    prey.insert(prey.end(), new_prey.begin(), new_prey.end());

    // Depredadores
    std::vector<Predator> new_pred;
    for (auto& d : predators) {
        if (d.alive) d.update(prey, pred_vision_radius, pred_vision_angle, pred_move_cost, p_catch);
        if (d.alive) { Predator* child = d.try_reproduce(); if (child) new_pred.push_back(*child); }
    }
    predators.erase(std::remove_if(predators.begin(), predators.end(), [](const Predator& d){ return !d.alive; }), predators.end());
    predators.insert(predators.end(), new_pred.begin(), new_pred.end());

    step++;
    int n_grass = 0;
    for (auto& g : grass) if (g.alive) n_grass++;
    graph.push(predators.size(), prey.size(), n_grass);

    update_ui();
    if (save_timer > 0) save_timer--;
}

// ===========================================================================
// Renderizado
// ===========================================================================

void Sim::draw_grid() {
    fl_color(GRID_COLOR);
    for (int x = 0; x < WIDTH; x += 40) fl_line(x, 0, x, HEIGHT);
    for (int y = 0; y < HEIGHT; y += 40) fl_line(0, y, WIDTH, y);
}

void Sim::draw() {
    // 1. Fondo del panel
    fl_push_clip(WIDTH, 0, PANEL_W, HEIGHT);
    fl_color(UI_BG);
    fl_rectf(WIDTH, 0, PANEL_W, HEIGHT);
    fl_color(GRID_COLOR);
    fl_line(WIDTH, 0, WIDTH, HEIGHT);
    fl_pop_clip();

    // 2. Widgets hijos (sliders, botones)
    Fl_Double_Window::draw();

    // 3. Área de simulación
    fl_push_clip(0, 0, WIDTH, HEIGHT);
    fl_color(BG_COLOR);
    fl_rectf(0, 0, WIDTH, HEIGHT);
    draw_grid();

    if (show_vision) {
        for (auto& p : prey) p.draw_vision((int)p.x, (int)p.y, true);
        for (auto& d : predators) d.draw_vision((int)d.x, (int)d.y, true);
    }

    for (auto& g : grass) g.draw((int)g.x, (int)g.y);
    for (auto& p : prey) p.draw((int)p.x, (int)p.y);
    for (auto& d : predators) d.draw((int)d.x, (int)d.y);

    if (paused) {
        fl_color(fl_rgb_color(0, 0, 0));
        fl_rectf(WIDTH/2 - 50, HEIGHT/2 - 15, 100, 30);
        fl_color(WHITE);
        fl_rect(WIDTH/2 - 50, HEIGHT/2 - 15, 100, 30);
        fl_draw("PAUSADO", WIDTH/2 - 30, HEIGHT/2 + 3);
    }

    if (save_timer > 0 && !save_msg.empty()) {
        fl_color(fl_rgb_color(100, 240, 120));
        fl_draw(save_msg.c_str(), 10, HEIGHT - 10);
    }
    fl_pop_clip();

    // 4. Texto del panel (encima de todo)
    fl_push_clip(WIDTH, 0, PANEL_W, HEIGHT);
    {
        char buf[64];
        int n_grass_alive = 0;
        for (auto& g : grass) if (g.alive) n_grass_alive++;

        fl_color(FL_BLACK);
        fl_font(FL_HELVETICA_BOLD, 14);
        fl_draw("EcoSim", WIDTH + PANEL_W/2 - 28, 22);

        fl_font(FL_HELVETICA_BOLD, 12);

        fl_color(PRED_COLOR);  fl_draw("Depredadores:", WIDTH + 10, 44);
        snprintf(buf, sizeof(buf), "%zu", predators.size());
        fl_color(FL_BLACK);    fl_draw(buf, WIDTH + 105, 44);

        fl_color(PREY_COLOR);  fl_draw("Presas:", WIDTH + 10, 60);
        snprintf(buf, sizeof(buf), "%zu", prey.size());
        fl_color(FL_BLACK);    fl_draw(buf, WIDTH + 60, 60);

        fl_color(GRASS_COLOR); fl_draw("Autótrofos:", WIDTH + 10, 76);
        snprintf(buf, sizeof(buf), "%d", n_grass_alive);
        fl_color(FL_BLACK);    fl_draw(buf, WIDTH + 84, 76);

        fl_color(FL_BLACK);
        fl_font(FL_HELVETICA, 11);
        fl_draw("Iteración =", WIDTH + 10, 92);
        snprintf(buf, sizeof(buf), "%d", step);
        fl_draw(buf, WIDTH + 70, 92);

        // Etiquetas de sliders con valor actual
        struct { const char* name; double lo, hi; } sinfo[8] = {
            {"p_captura", 0.05, 1.0},
            {"radio dep.", 40, 250},
            {"angulo dep.", 10, 360},
            {"radio presa", 30, 200},
            {"angulo presa", 10, 360},
            {"regrowth", 50, 400},
            {"costo dep.", 0.05, 0.8},
            {"costo presa", 0.05, 0.6},
        };

        fl_font(FL_HELVETICA_BOLD, 11);
        int base_y = 105;
        for (int i = 0; i < 8; i++) {
            int label_y = base_y + i * 45 + 9;
            double val  = sliders[i]->value();

            bool is_int = (sinfo[i].hi - sinfo[i].lo) >= 50.0 && sinfo[i].lo >= 1.0;
            if (is_int)
                snprintf(buf, sizeof(buf), "%s [%.0f-%.0f]: %.0f",
                         sinfo[i].name, sinfo[i].lo, sinfo[i].hi, val);
            else
                snprintf(buf, sizeof(buf), "%s [%.2f-%.1f]: %.3f",
                         sinfo[i].name, sinfo[i].lo, sinfo[i].hi, val);

            fl_color(FL_BLACK);
            fl_draw(buf, WIDTH + 10, label_y);
        }
    }
    fl_pop_clip();

    // 5. Gráfica al fondo del panel
    int graph_h = 90;
    graph.draw(WIDTH + 8, HEIGHT - graph_h - 4, PANEL_W - 16, graph_h);
}

// ===========================================================================
// Guardado (TXT + CSV + Gnuplot)
// ===========================================================================

void Sim::do_save() {
    mkdir("output", 0755);

    std::string base = "output/ecosim_" + std::to_string(step);
    std::string txt_path = base + ".txt";
    std::string csv_path = base + "_hist.csv";
    std::string gp_path  = base + ".gp";

    // TXT
    std::ofstream file(txt_path);
    if (!file.is_open()) { save_msg = "Error al guardar"; save_timer = 180; return; }

    std::time_t t = std::time(nullptr);
    char tbuf[64];
    std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    file << "=== EcoSim Report ===\n";
    file << "Fecha:  " << tbuf << "\n";
    file << "Semilla RNG: " << RANDOM_SEED << "\n";
    file << "Paso:   " << step << "\n\n";

    file << "--- Poblaciones finales ---\n";
    file << "Depredadores: " << predators.size() << "\n";
    file << "Presas: " << prey.size() << "\n";
    int n_grass = 0;
    for (auto& g : grass) if (g.alive) n_grass++;
    file << "Hierba viva: " << n_grass << "\n\n";

    file << "--- Parametros activos ---\n";
    file << std::fixed << std::setprecision(4);
    file << "p_captura: " << p_catch << "\n";
    file << "radio_vision_dep: " << pred_vision_radius << "\n";
    file << "angulo_vision_dep: " << pred_vision_angle << " deg\n";
    file << "radio_vision_presa: " << prey_vision_radius << "\n";
    file << "angulo_vision_presa: " << prey_vision_angle << " deg\n";
    file << "costo_movimiento_dep: " << pred_move_cost << "\n";
    file << "costo_movimiento_presa: " << prey_move_cost << "\n";
    file << "tiempo_regrowth: " << grass_regrow_time << "\n\n";
    file.close();

    // CSV
    bool have_hist = !graph.pred_full.empty();
    if (have_hist) {
        std::ofstream csv(csv_path);
        csv << "paso,depredadores,presas,autotrofos\n";
        size_t n = graph.pred_full.size();
        for (size_t i = 0; i < n; i++)
            csv << i << "," << graph.pred_full[i] << "," << graph.prey_full[i] << "," << graph.grass_full[i] << "\n";
        csv.close();
    }

    // Gnuplot
    if (have_hist) {
        std::string png_dyn = base + "_dinamicas.png";
        std::string png_dp  = base + "_fase_dep_presa.png";
        std::string png_pa  = base + "_fase_presa_aut.png";
        std::string png_da  = base + "_fase_dep_aut.png";
        size_t nrows = graph.pred_full.size();

        std::ofstream gp(gp_path);
        gp << "set terminal pngcairo size 900,560 enhanced font 'Sans,11'\n";
        gp << "set datafile separator ','\n";
        gp << "set grid lc rgb '#cccccc'\n";

        // Dinámicas temporales
        gp << "set output '" << png_dyn << "'\n";
        gp << "set title 'Dinamicas de poblacion  (semilla=" << RANDOM_SEED << ", t=" << step << ")'\n";
        gp << "set xlabel 'Paso'\nset ylabel 'Individuos'\nset key top right\nunset colorbox\n";
        gp << "plot '" << csv_path << "' using 1:2 with lines lw 2 lc rgb '#DC3C3C' title 'Depredadores', \\\n";
        gp << " '" << csv_path << "' using 1:3 with lines lw 2 lc rgb '#3C82DC' title 'Presas', \\\n";
        gp << " '" << csv_path << "' using 1:4 with lines lw 2 lc rgb '#3CB450' title 'Autotrofos'\n";

        auto phase_plot = [&](const std::string& outfile, const std::string& title,
                               const std::string& xlabel, const std::string& ylabel,
                               int xcol, int ycol) {
            gp << "set output '" << outfile << "'\n";
            gp << "set title '" << title << "'\n";
            gp << "set xlabel '" << xlabel << "'\nset ylabel '" << ylabel << "'\n";
            gp << "set palette defined (0 '#1a1a6e', 25 '#2166ac', 50 '#4dac26', 75 '#f4a582', 100 '#d6604d')\n";
            gp << "set cbrange [0:" << nrows - 1 << "]\n";
            gp << "set colorbox vertical user origin 0.88,0.15 size 0.03,0.70\n";
            gp << "set cblabel 'Iteracion'\nunset key\n";
            gp << "plot '" << csv_path << "' skip 1 using " << xcol << ":" << ycol << ":1 with lines lw 1.5 lc palette notitle, \\\n";
            gp << " '" << csv_path << "' every ::1::1 using " << xcol << ":" << ycol << " with points pt 7 ps 1.8 lc rgb '#00cc44' title 'Inicio', \\\n";
            gp << "     '" << csv_path << "' every ::" << nrows << "::" << nrows << " using " << xcol << ":" << ycol << " with points pt 7 ps 1.8 lc rgb '#ee2222' title 'Final'\n";
        };

        phase_plot(png_dp, "Diagrama de fase: Depredadores vs Presas  (semilla=" + std::to_string(RANDOM_SEED) + ")", "Presas", "Depredadores", 3, 2);
        phase_plot(png_pa, "Diagrama de fase: Presas vs Autotrofos  (semilla=" + std::to_string(RANDOM_SEED) + ")",  "Autotrofos", "Presas", 4, 3);
        phase_plot(png_da, "Diagrama de fase: Depredadores vs Autotrofos  (semilla=" + std::to_string(RANDOM_SEED) + ")", "Autotrofos", "Depredadores", 4, 2);
        gp.close();

        int ret = system(("gnuplot " + gp_path).c_str());
        save_msg = (ret == 0) ? "Guardado en output/ (txt+csv+4 PNG)" : "Guardado txt+csv  (gnuplot fallido)";
    } else {
        save_msg = "Guardado: " + txt_path + "  (sin historial)";
    }
    save_timer = 240;
}

// ===========================================================================
// Eventos de teclado y run
// ===========================================================================

int Sim::handle(int event) {
    if (event == FL_KEYDOWN) {
        int key = Fl::event_key();
        switch (key) {
            case 'r': case 'R': reset();  return 1;
            case 'p': case 'P':
                paused = !paused;
                btn_pause->label(paused ? "[P] Reanudar" : "[P] Pausa");
                return 1;
            case 'v': case 'V':
                show_vision = !show_vision;
                btn_vision->label(show_vision ? "[V] Ocultar" : "[V] Vision");
                return 1;
            case 's': case 'S': do_save(); return 1;
            case FL_Escape: hide();    return 1;
            default: break;
        }
    }
    return Fl_Double_Window::handle(event);
}

void Sim::run() {
    Fl::run();
}
