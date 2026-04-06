/**
 ecosim.h
 * Definiciones y declaraciones del modelo basado en agentes
 */

#ifndef ECOSIM_H
#define ECOSIM_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/fl_draw.H>
#include <cmath>
#include <vector>
#include <deque>
#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cstring>

// ===========================================================================
// PARAMETROS CONSTANTES
// ===========================================================================

const int WIDTH = 900;
const int HEIGHT = 620;
const int PANEL_W = 260; // En este tamaño entran sliders y etiquetas
const int FPS = 60;

const int RANDOM_SEED = 42;

const int N_PREDATORS = 3;
const int N_PREY = 25;
const int N_GRASS = 350;

const double PRED_SPEED = 1.8;
const double PREY_SPEED = 1.5;

const double PRED_VISION_RADIUS = 118.0;
const double PREY_VISION_RADIUS = 90.0;
const double PRED_VISION_ANGLE = 68.0;
const double PREY_VISION_ANGLE = 124.0;

const double INIT_ENERGY = 100.0;
const double PRED_MOVE_COST = 0.3738;
const double PREY_MOVE_COST = 0.19;
const double PRED_ENERGY_GAIN = 60.0;
const double PREY_ENERGY_GAIN = 30.0;

const double P_CATCH = 0.230;

const double PRED_REPRO_THRESHOLD = 160.0;
const double PREY_REPRO_THRESHOLD = 130.0;
const double PRED_REPRO_PROB = 0.0052;
const double PREY_REPRO_PROB = 0.008;

const int GRASS_REGROW_TIME = 180;
const double P_TURN = 0.02;

const int PRED_HEAD_SIZE = 12;
const int PREY_HEAD_SIZE = 9;
const int GRASS_SIZE = 7;

const int N_BODY_SEGS = 5;
const int SEG_GAP = 5;

const int HISTORY_LEN = 400;

// Colores
const Fl_Color BG_COLOR = fl_rgb_color(12, 14, 28);
const Fl_Color GRID_COLOR = fl_rgb_color(28, 32, 55);
const Fl_Color PRED_COLOR = fl_rgb_color(220, 60, 60);
const Fl_Color PREY_COLOR = fl_rgb_color(60, 130, 220);
const Fl_Color GRASS_COLOR = fl_rgb_color(60, 180, 80);
const Fl_Color UI_BG = fl_rgb_color(18, 20, 42);
const Fl_Color WHITE = fl_rgb_color(240, 240, 240);
const Fl_Color GRAY = fl_rgb_color(130, 130, 160);

// Funciones auxiliares para componentes RGB
inline unsigned char fl_color_red(Fl_Color c) {
    return (c >> 24) & 0xFF;
}

inline unsigned char fl_color_green(Fl_Color c) {
    return (c >> 16) & 0xFF;
}

inline unsigned char fl_color_blue(Fl_Color c) {
    return (c >> 8) & 0xFF;
}

// ===========================================================================
// Funciones auxiliares
// ===========================================================================

// RNG global reseedable - se resetea a RANDOM_SEED en cada reset()
inline std::mt19937& global_rng() {
    static std::mt19937 rng(RANDOM_SEED);
    return rng;
}

inline void reseed_rng() {
    global_rng().seed(RANDOM_SEED);
}

inline double random_double(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(global_rng());
}

inline int random_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(global_rng());
}

inline void wrap(double& x, double& y) {
    if (x < 0) x += WIDTH;
    if (x >= WIDTH) x -= WIDTH;
    if (y < 0) y += HEIGHT;
    if (y >= HEIGHT) y -= HEIGHT;
}

inline double dist(double ax, double ay, double bx, double by) {
    double dx = ax - bx;
    double dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
}

inline double angle_towards(double ax, double ay, double bx, double by) {
    return std::atan2(by - ay, bx - ax);
}

inline double angle_away(double ax, double ay, double bx, double by) {
    return std::atan2(ay - by, ax - bx);
}

inline double random_angle() {
    return random_double(0, 2 * M_PI);
}

inline double angle_diff(double a, double b) {
    double d = std::fabs(a - b);
    d = std::fmod(d, 2 * M_PI);
    if (d > M_PI) d = 2 * M_PI - d;
    return d;
}

inline bool in_cone(double agent_angle, double agent_x, double agent_y,
                    double tx, double ty, double half_angle_rad) {
    double to_target = std::atan2(ty - agent_y, tx - agent_x);
    return angle_diff(agent_angle, to_target) <= half_angle_rad;
}

// ===========================================================================
// Clase Grass
// ===========================================================================

class Grass {
public:
    double x, y;
    bool alive;
    int regrow_timer;

    Grass(double x, double y);
    void update(int regrow_time);
    void eat(int regrow_time);
    void draw(int cx, int cy);
};

// ===========================================================================
// Clase Agent
// ===========================================================================

class Agent {
public:
    double x, y;
    Fl_Color color;
    int head_size;
    double speed;
    double vision_r;
    double vision_a_deg;
    double move_cost;
    double energy;
    bool alive;
    double angle;
    std::deque<std::pair<double, double>> body;
    double accum;

    Agent(double x, double y, Fl_Color color, int head_size, double speed,
          double vision_r, double vision_a_deg, double move_cost);
    
    virtual ~Agent() = default;
    
    bool can_see(double tx, double ty);
    void step(double new_angle = -999.0);
    void draw(int cx, int cy);
    void draw_vision(int cx, int cy, bool show);
};

// ===========================================================================
// Clase Prey
// ===========================================================================

class Prey : public Agent {
public:
    Prey(double x, double y);
    void update(std::vector<Grass>& grass, std::vector<class Predator>& predators,
                double prey_vr, double prey_va, double prey_mc, int grass_regrow);
    Prey* try_reproduce();
};

// ===========================================================================
// Clase Predator
// ===========================================================================

class Predator : public Agent {
public:
    Predator(double x, double y);
    void update(std::vector<Prey>& prey, double pred_vr, double pred_va,
                double pred_mc, double p_catch);
    Predator* try_reproduce();
};

// ===========================================================================
// Clase PopGraph
// ===========================================================================

class PopGraph {
public:
    std::deque<int> pred_hist;
    std::deque<int> prey_hist;
    std::deque<int> grass_hist;
    std::vector<int> pred_full;
    std::vector<int> prey_full;
    std::vector<int> grass_full;
    
    PopGraph();
    void push(int np, int ny, int ng);
    void draw(int x, int y, int w, int h);
};

// ===========================================================================
// Clase Sim
// ===========================================================================

class Sim : public Fl_Double_Window {
private:
    // Parámetros ajustables
    double p_catch;
    double pred_vision_radius;
    double pred_vision_angle;
    double prey_vision_radius;
    double prey_vision_angle;
    int grass_regrow_time;
    double pred_move_cost;
    double prey_move_cost;
    
    // Estado
    bool paused;
    bool show_vision;
    int step;
    int save_timer;
    std::string save_msg;
    
    // Entidades
    std::vector<Grass> grass;
    std::vector<Prey> prey;
    std::vector<Predator> predators;
    
    // UI
    PopGraph graph;
    std::vector<Fl_Slider*> sliders;
    Fl_Output* pred_count;
    Fl_Output* prey_count;
    Fl_Output* grass_count;
    Fl_Output* step_count;
    Fl_Button* btn_reset;
    Fl_Button* btn_pause;
    Fl_Button* btn_vision;
    Fl_Button* btn_save;
    
    // Callbacks
    static void timer_cb(void* data);
    static void reset_cb(Fl_Widget* w, void* data);
    static void pause_cb(Fl_Widget* w, void* data);
    static void vision_cb(Fl_Widget* w, void* data);
    static void save_cb(Fl_Widget* w, void* data);
    static void slider_cb(Fl_Widget* w, void* data);
    
    void update_ui();
    void reset();
    void do_save();
    void draw_grid();
    
public:
    Sim();
    ~Sim();
    void update();
    void draw();
    void run();
    int handle(int event) override;
    
    // Getters
    double get_p_catch() const { return p_catch; }
    double get_pred_vision_radius() const { return pred_vision_radius; }
    double get_pred_vision_angle() const { return pred_vision_angle; }
    double get_prey_vision_radius() const { return prey_vision_radius; }
    double get_prey_vision_angle() const { return prey_vision_angle; }
    int get_grass_regrow_time() const { return grass_regrow_time; }
    double get_pred_move_cost() const { return pred_move_cost; }
    double get_prey_move_cost() const { return prey_move_cost; }
    
    // Setters
    void set_p_catch(double v) { p_catch = v; }
    void set_pred_vision_radius(double v) { pred_vision_radius = v; }
    void set_pred_vision_angle(double v) { pred_vision_angle = v; }
    void set_prey_vision_radius(double v) { prey_vision_radius = v; }
    void set_prey_vision_angle(double v) { prey_vision_angle = v; }
    void set_grass_regrow_time(int v) { grass_regrow_time = v; }
    void set_pred_move_cost(double v) { pred_move_cost = v; }
    void set_prey_move_cost(double v) { prey_move_cost = v; }
};

#endif // ECOSIM_H
