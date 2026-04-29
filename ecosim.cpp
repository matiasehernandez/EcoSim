/*
 * ecosim.cpp -- Modelo basado en agentes: Autotrofos / Presas / Depredadores
 * ============================================================
 * Controles:
 *   [R]   Reiniciar
 *   [P]   Pausa / reanudar
 *   [V]   Mostrar/ocultar conos de vision
 *   [S]   Guardar TXT de metricas
 *   [Esc] Salir
 *
 * Compilar:
 *   Usar Makefile o a mano: g++ -O2 -o ecosim ecosim.cpp $(fltk-config --cxxflags --ldflags) -lm
 */

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ===========================================================================
// PARAMETROS GLOBALES
// ===========================================================================

static const int   SIM_W = 900;
static const int   SIM_H = 620;
static const int   PANEL_W = 220;
static const int   GRAPH_H = 90;  // altura de la grafica de poblacion
static const int   WIN_W = SIM_W + PANEL_W;
static const int   WIN_H = SIM_H;
static const int   FPS_TARGET  = 60;
static const int   RANDOM_SEED = 42;
static const int   T_F = 25000;   // -1 = indefinido, nunca para

// Poblaciones iniciales
static const int   N_PREDATORS = 6;
static const int   N_PREY = 15;
static const int   N_PREY2 = 15;
static const int   N_GRASS = 200;
static const int   N_GRASS2 = 150;

// Velocidades
static const float PRED_SPEED = 1.8f;
static const float PREY_SPEED = 1.5f;
static const float PREY2_SPEED = 1.56f;

// Visión
static const float PRED_VISION_RADIUS = 118.f;
static const float PREY_VISION_RADIUS = 90.f;
static const float PREY2_VISION_RADIUS = 85.f;
static const float PRED_VISION_ANGLE = 68.f;
static const float PREY_VISION_ANGLE = 124.f;
static const float PREY2_VISION_ANGLE = 110.f;

// Energía
static const float INIT_ENERGY = 100.f;
static const float PRED_MOVE_COST = 0.373625f;
static const float PREY_MOVE_COST = 0.20f;
static const float PREY2_MOVE_COST = 0.1854f;
static const float PRED_ENERGY_GAIN = 60.f;
static const float PREY_ENERGY_GAIN = 30.f;
static const float PREY2_ENERGY_GAIN = 25.f;

static const float P_CATCH = 0.230f;

// Reproducción
static const float PRED_REPRO_THRESHOLD = 165.f;
static const float PREY_REPRO_THRESHOLD = 140.f;
static const float PREY2_REPRO_THRESHOLD = 130.f;
static const float PRED_REPRO_PROB = 0.0055f;
static const float PREY_REPRO_PROB = 0.008f;
static const float PREY2_REPRO_PROB = 0.0071f;

// Autótrofos
static const int   GRASS_REGROW_TIME = 180;
static const int   GRASS2_REGROW_TIME = 150;
static const float P_TURN = 0.02f;

// Sprites
static const int PRED_HEAD_SIZE = 12;
static const int PREY_HEAD_SIZE = 9;
static const int PREY2_HEAD_SIZE = 7;
static const int GRASS_SIZE = 7;
static const int GRASS2_SIZE = 5;

// Cuerpo
static const int N_BODY_SEGS = 5;
static const int SEG_GAP = 5;

// Colores FLTK (fl_rgb_color)
static Fl_Color COL_BG;
static Fl_Color COL_GRID;
static Fl_Color COL_PRED;
static Fl_Color COL_PREY;
static Fl_Color COL_PREY2;
static Fl_Color COL_GRASS;
static Fl_Color COL_GRASS2;
static Fl_Color COL_UI_BG;
static Fl_Color COL_WHITE;
static Fl_Color COL_GRAY;

static void init_colors() {
    COL_BG = fl_rgb_color(12, 14, 28);
    COL_GRID = fl_rgb_color(28, 32, 55);
    COL_PRED = fl_rgb_color(220, 60, 60);
    COL_PREY = fl_rgb_color(60, 130, 220);
    COL_PREY2 = fl_rgb_color(140, 140, 150);
    COL_GRASS = fl_rgb_color(60, 180, 80);
    COL_GRASS2= fl_rgb_color(20, 100, 30);
    COL_UI_BG = fl_rgb_color(18, 20, 42);
    COL_WHITE = fl_rgb_color(240, 240, 240);
    COL_GRAY = fl_rgb_color(130, 130, 160);
}

// ===========================================================================
// RNG global
// ===========================================================================
static std::mt19937 rng(RANDOM_SEED);

static float randf(){ return std::uniform_real_distribution<float>(0,1)(rng); }
static float randf(float a,float b){ return std::uniform_real_distribution<float>(a,b)(rng); }
static float rand_angle(){ return randf(0.f, 2.f*(float)M_PI); }

// ===========================================================================
// Utilidades geométricas
// ===========================================================================

static inline void wrap(float &x, float &y) {
    if (x < 0) x += SIM_W; else if (x >= SIM_W) x -= SIM_W;
    if (y < 0) y += SIM_H; else if (y >= SIM_H) y -= SIM_H;
}

static inline float dist2(float ax,float ay,float bx,float by){
    float dx=ax-bx, dy=ay-by; return dx*dx+dy*dy;
}
static inline float dist_(float ax,float ay,float bx,float by){
    return std::sqrt(dist2(ax,ay,bx,by));
}
static inline float angle_towards(float ax,float ay,float bx,float by){
    return std::atan2(by-ay, bx-ax);
}
static inline float angle_away(float ax,float ay,float bx,float by){
    return std::atan2(ay-by, ax-bx);
}
static inline float angle_diff(float a,float b){
    float d = std::fabs(a-b);
    d = std::fmod(d, 2.f*(float)M_PI);
    return d <= (float)M_PI ? d : 2.f*(float)M_PI - d;
}
static inline bool in_cone(float agent_angle, float ax, float ay,
                             float tx, float ty, float half_rad){
    float to = std::atan2(ty-ay, tx-ax);
    return angle_diff(agent_angle, to) <= half_rad;
}

// Clamp color component
static inline uchar clamp_col(int v){ return (uchar)(v<0?0:v>255?255:v); }

// Brighter color
static inline Fl_Color brighter(Fl_Color c, int delta){
    uchar r,g,b;
    Fl::get_color(c,r,g,b);
    return fl_rgb_color(clamp_col(r+delta), clamp_col(g+delta), clamp_col(b+delta));
}

// Scaled color (fraction 0..1)
static inline Fl_Color scaled_color(Fl_Color c, float frac){
    uchar r,g,b;
    Fl::get_color(c,r,g,b);
    return fl_rgb_color(
        (uchar)std::max(20,(int)(r*frac)),
        (uchar)std::max(20,(int)(g*frac)),
        (uchar)std::max(20,(int)(b*frac))
    );
}

// ===========================================================================
// CLASE: Grass
// ===========================================================================

struct Grass {
    float x, y;
    Fl_Color color, bright_color;
    int size;
    int regrow_time;
    bool alive;
    int regrow_timer;

    Grass(float x_, float y_, Fl_Color col, int sz, int rt)
        : x(x_), y(y_), color(col), size(sz), regrow_time(rt),
          alive(true), regrow_timer(0)
    {
        bright_color = brighter(col, 60);
    }

    void update(){
        if(!alive){
            if(--regrow_timer <= 0) alive = true;
        }
    }

    void eat(int rt_override){
        alive = false;
        regrow_timer = rt_override;
    }

    void draw() const {
        if(!alive) return;
        int s  = size;
        int cx = (int)x - s/2;
        int cy = (int)y - s/2;
        fl_color(color);
        fl_rectf(cx, cy, s, s);
        int inner = std::max(2, s-4);
        fl_color(bright_color);
        fl_rectf((int)x - inner/2, (int)y - inner/2, inner, inner);
    }
};

// ===========================================================================
// CLASE: Agent (base)
// ===========================================================================

struct BodyPos { float x, y; };

struct Agent {
    float x, y;
    float angle;
    float energy;
    bool alive;
    Fl_Color color;
    int head_size;
    float speed;
    float vision_r;
    float vision_a_deg;
    float move_cost;

    // Cuerpo segmentado
    std::deque<BodyPos> body;
    float _accum;

    // Cache de segmentos (constante)
    struct SegInfo { int size; Fl_Color color; };
    SegInfo segs[N_BODY_SEGS];
    Fl_Color bright_color;
    int inner_size;

    // Cache semi-angulo
    float _half_angle_rad;
    float _vision_a_cached;

    Agent(float x_, float y_, Fl_Color col, int hs, float spd,
          float vr, float va, float mc)
        : x(x_), y(y_), angle(rand_angle()), energy(INIT_ENERGY), alive(true),
          color(col), head_size(hs), speed(spd),
          vision_r(vr), vision_a_deg(va), move_cost(mc), _accum(0.f)
    {
        BodyPos bp{x,y};
        for(int i=0;i<N_BODY_SEGS;i++) body.push_back(bp);

        // Pre-calcular segmentos
        for(int i=0;i<N_BODY_SEGS;i++){
            float frac = 1.f - (i+1.f)/(N_BODY_SEGS+1.f);
            segs[i].size = std::max(2,(int)(head_size*(0.35f+0.55f*frac)));
            segs[i].color = scaled_color(col, 0.35f+0.55f*frac);
        }
        bright_color = brighter(col, 70);
        inner_size = std::max(2, head_size-4);

        _half_angle_rad  = (float)(va * M_PI / 360.0);
        _vision_a_cached = va;
    }

    bool can_see(float tx, float ty){
        if(dist2(x,y,tx,ty) > vision_r*vision_r) return false;
        if(vision_a_deg != _vision_a_cached){
            _half_angle_rad  = (float)(vision_a_deg * M_PI / 360.0);
            _vision_a_cached = vision_a_deg;
        }
        return in_cone(angle, x, y, tx, ty, _half_angle_rad);
    }

    void step(float new_angle = -999.f){
        if(new_angle > -998.f) angle = new_angle;
        else if(randf() < P_TURN) angle += randf(-0.6f, 0.6f);

        x += std::cos(angle)*speed;
        y += std::sin(angle)*speed;
        wrap(x, y);
        energy -= move_cost;
        if(energy <= 0.f) alive = false;

        _accum += speed;
        if(_accum >= SEG_GAP){
            body.push_front({x,y});
            if((int)body.size() > N_BODY_SEGS) body.pop_back();
            _accum = 0.f;
        }
    }

    void draw_agent() const {
        if(!alive) return;
        // Cola (atras → adelante)
        int n = (int)body.size();
        for(int i=n-1; i>=0; i--){
            int s  = (i < N_BODY_SEGS) ? segs[i].size  : segs[N_BODY_SEGS-1].size;
            Fl_Color sc = (i < N_BODY_SEGS) ? segs[i].color : segs[N_BODY_SEGS-1].color;
            fl_color(sc);
            fl_rectf((int)body[i].x - s/2, (int)body[i].y - s/2, s, s);
        }
        // Cabeza
        fl_color(color);
        fl_rectf((int)x - head_size/2, (int)y - head_size/2, head_size, head_size);
        fl_color(bright_color);
        fl_rectf((int)x - inner_size/2, (int)y - inner_size/2, inner_size, inner_size);
    }

    void draw_vision_cone() const {
        if(!alive) return;
        float r = vision_r;
        float half = (float)(vision_a_deg * M_PI / 360.0);
        int cx = (int)x;
        int cy = (int)y;
        int steps= std::max(12, (int)(vision_a_deg/6.f));

        uchar cr,cg,cb;
        Fl::get_color(color, cr, cg, cb);

        // Relleno del sector con alpha simulado (color oscuro semitransparente)
        // FLTK no tiene alpha nativo: usar color muy oscuro mezclado (ver documentación de FLTK)
        Fl_Color fill_col = fl_rgb_color(
            clamp_col((int)(cr*0.07f + 12*0.93f)),
            clamp_col((int)(cg*0.07f + 14*0.93f)),
            clamp_col((int)(cb*0.07f + 28*0.93f))
        );
        Fl_Color edge_col = fl_rgb_color(cr/4, cg/4, cb/4);

        fl_color(fill_col);
        // Poligono como tira de triángulos desde el centro
        fl_begin_complex_polygon();
        fl_vertex(cx, cy);
        for(int i=0;i<=steps;i++){
            float a = angle - half + (2.f*half)*(i/(float)steps);
            fl_vertex(cx + (int)(r*std::cos(a)),
                      cy + (int)(r*std::sin(a)));
        }
        fl_end_complex_polygon();

        // Bordes
        fl_color(edge_col);
        fl_line(cx, cy,
                cx + (int)(r*std::cos(angle-half)),
                cy + (int)(r*std::sin(angle-half)));
        fl_line(cx, cy,
                cx + (int)(r*std::cos(angle+half)),
                cy + (int)(r*std::sin(angle+half)));
        // Arco exterior
        fl_arc((float)(cx-r), (float)(cy-r),
               (float)(2*r),  (float)(2*r),
               (double)((-angle-half)*180.0/M_PI),
               (double)((-angle+half)*180.0/M_PI));
    }
};

// ===========================================================================
// Parámetros ajustables en tiempo real (sliders)
// ===========================================================================

struct SimParams {
    float p_catch = P_CATCH;
    float pred_vision_radius = PRED_VISION_RADIUS;
    float pred_vision_angle  = PRED_VISION_ANGLE;
    float prey_vision_radius = PREY_VISION_RADIUS;
    float prey_vision_angle  = PREY_VISION_ANGLE;
    float prey2_vision_radius= PREY2_VISION_RADIUS;
    float prey2_vision_angle = PREY2_VISION_ANGLE;
    float grass_regrow_time  = GRASS_REGROW_TIME;
    float grass2_regrow_time = GRASS2_REGROW_TIME;
    float pred_move_cost = PRED_MOVE_COST;
    float prey_move_cost = PREY_MOVE_COST;
    float prey2_move_cost = PREY2_MOVE_COST;
};

// ===========================================================================
// CLASE: Sim (logica de simulacion)
// ===========================================================================

struct PopRecord {
    int t, pred, prey, prey2, grass, grass2;
};

struct Sim {
    std::vector<Grass>  grass, grass2;
    std::vector<Agent>  prey, prey2, predators;
    int   step = 0;
    bool  paused = false;
    bool  show_vision = false;
    SimParams p;

    // Conteos cacheados
    int n_pred=0, n_prey=0, n_prey2=0, n_grass=0, n_grass2=0;

    // Historia temporal para gráficas
    std::vector<PopRecord> history;
    static const int RECORD_EVERY = 10; // guardar cada N pasos

    void reset(){
        rng.seed(RANDOM_SEED);
        grass.clear(); grass2.clear();
        prey.clear();  prey2.clear(); predators.clear();
        history.clear();
        step = 0;

        for(int i=0;i<N_GRASS;i++)
            grass.emplace_back(randf(0,SIM_W-1), randf(0,SIM_H-1),
                               COL_GRASS, GRASS_SIZE, (int)p.grass_regrow_time);
        for(int i=0;i<N_GRASS2;i++)
            grass2.emplace_back(randf(0,SIM_W-1), randf(0,SIM_H-1),
                                COL_GRASS2, GRASS2_SIZE, (int)p.grass2_regrow_time);

        for(int i=0;i<N_PREY;i++)
            prey.emplace_back(randf(0,SIM_W-1), randf(0,SIM_H-1),
                              COL_PREY, PREY_HEAD_SIZE, PREY_SPEED,
                              p.prey_vision_radius, p.prey_vision_angle, p.prey_move_cost);
        for(int i=0;i<N_PREY2;i++)
            prey2.emplace_back(randf(0,SIM_W-1), randf(0,SIM_H-1),
                               COL_PREY2, PREY2_HEAD_SIZE, PREY2_SPEED,
                               p.prey2_vision_radius, p.prey2_vision_angle, p.prey2_move_cost);
        for(int i=0;i<N_PREDATORS;i++)
            predators.emplace_back(randf(0,SIM_W-1), randf(0,SIM_H-1),
                                   COL_PRED, PRED_HEAD_SIZE, PRED_SPEED,
                                   p.pred_vision_radius, p.pred_vision_angle, p.pred_move_cost);

        n_pred=N_PREDATORS; n_prey=N_PREY; n_prey2=N_PREY2;
        n_grass=N_GRASS; n_grass2=N_GRASS2;
    }

    // -- update hierba
    void update_grass(){
        for(auto &g : grass)  g.update();
        for(auto &g : grass2) g.update();
    }

    // -- actualizar presa tipo 1
    void update_prey_agent(Agent &ag){
        ag.vision_r = p.prey_vision_radius;
        ag.vision_a_deg = p.prey_vision_angle;
        ag.move_cost = p.prey_move_cost;

        // Huir del depredador mas cercano
        Agent *flee = nullptr;
        float  min_d = ag.vision_r;
        for(auto &d : predators){
            if(!d.alive) continue;
            if(ag.can_see(d.x,d.y)){
                float dd = dist_(ag.x,ag.y,d.x,d.y);
                if(dd < min_d){ min_d=dd; flee=&d; }
            }
        }
        if(flee){ ag.step(angle_away(ag.x,ag.y,flee->x,flee->y)); return; }

        // Buscar hierba tipo 1
        Grass *food = nullptr;
        float  min_fd = ag.vision_r;
        for(auto &g : grass){
            if(!g.alive) continue;
            if(ag.can_see(g.x,g.y)){
                float dd = dist_(ag.x,ag.y,g.x,g.y);
                if(dd < min_fd){ min_fd=dd; food=&g; }
            }
        }
        // Si no hay tipo 1, busca tipo 2 con 40%
        if(!food && randf() < 0.4f){
            for(auto &g : grass2){
                if(!g.alive) continue;
                if(ag.can_see(g.x,g.y)){
                    float dd = dist_(ag.x,ag.y,g.x,g.y);
                    if(dd < min_fd){ min_fd=dd; food=&g; }
                }
            }
        }
        if(food){
            ag.step(angle_towards(ag.x,ag.y,food->x,food->y));
            float cr = (float)(ag.head_size/2 + food->size/2 + 2);
            if(dist_(ag.x,ag.y,food->x,food->y) < cr){
                food->eat((int)p.grass_regrow_time);
                ag.energy = std::min(ag.energy + PREY_ENERGY_GAIN, 200.f);
            }
        } else {
            ag.step();
        }
    }

    // -- actualizar presa tipo 2
    void update_prey2_agent(Agent &ag){
        ag.vision_r  = p.prey2_vision_radius;
        ag.vision_a_deg = p.prey2_vision_angle;
        ag.move_cost = p.prey2_move_cost;

        Agent *flee = nullptr;
        float  min_d = ag.vision_r;
        for(auto &d : predators){
            if(!d.alive) continue;
            if(ag.can_see(d.x,d.y)){
                float dd = dist_(ag.x,ag.y,d.x,d.y);
                if(dd < min_d){ min_d=dd; flee=&d; }
            }
        }
        if(flee){ ag.step(angle_away(ag.x,ag.y,flee->x,flee->y)); return; }

        // Prefiere hierba tipo 2
        Grass *food = nullptr;
        float  min_fd = ag.vision_r;
        for(auto &g : grass2){
            if(!g.alive) continue;
            if(ag.can_see(g.x,g.y)){
                float dd = dist_(ag.x,ag.y,g.x,g.y);
                if(dd < min_fd){ min_fd=dd; food=&g; }
            }
        }
        if(!food && randf() < 0.4f){
            for(auto &g : grass){
                if(!g.alive) continue;
                if(ag.can_see(g.x,g.y)){
                    float dd = dist_(ag.x,ag.y,g.x,g.y);
                    if(dd < min_fd){ min_fd=dd; food=&g; }
                }
            }
        }
        if(food){
            ag.step(angle_towards(ag.x,ag.y,food->x,food->y));
            float cr = (float)(ag.head_size/2 + food->size/2 + 2);
            if(dist_(ag.x,ag.y,food->x,food->y) < cr){
                food->eat((int)p.grass2_regrow_time);
                ag.energy = std::min(ag.energy + PREY2_ENERGY_GAIN, 200.f);
            }
        } else {
            ag.step();
        }
    }

    // -- actualizar depredador
    void update_pred_agent(Agent &ag){
        ag.vision_r = p.pred_vision_radius;
        ag.vision_a_deg = p.pred_vision_angle;
        ag.move_cost = p.pred_move_cost;

        Agent *chase = nullptr;
        float  min_d = ag.vision_r;
        for(auto &pr : prey){
            if(!pr.alive) continue;
            if(ag.can_see(pr.x,pr.y)){
                float dd = dist_(ag.x,ag.y,pr.x,pr.y);
                if(dd < min_d){ min_d=dd; chase=&pr; }
            }
        }
        for(auto &pr : prey2){
            if(!pr.alive) continue;
            if(ag.can_see(pr.x,pr.y)){
                float dd = dist_(ag.x,ag.y,pr.x,pr.y);
                if(dd < min_d){ min_d=dd; chase=&pr; }
            }
        }
        if(chase){
            ag.step(angle_towards(ag.x,ag.y,chase->x,chase->y));
            float cr = (float)(ag.head_size/2 + chase->head_size/2 + 2);
            if(dist_(ag.x,ag.y,chase->x,chase->y) < cr){
                if(randf() < p.p_catch){
                    chase->alive = false;
                    ag.energy = std::min(ag.energy + PRED_ENERGY_GAIN, 250.f);
                }
            }
        } else {
            ag.step();
        }
    }

    void update(){
        if(paused) return;
        update_grass();

        // Presas tipo 1
        std::vector<Agent> new_prey;
        for(auto &ag : prey){
            if(ag.alive) update_prey_agent(ag);
            if(ag.alive && ag.energy > PREY_REPRO_THRESHOLD && randf() < PREY_REPRO_PROB){
                ag.energy *= 0.6f;
                float ox = ag.x + randf(-20,20);
                float oy = ag.y + randf(-20,20);
                wrap(ox,oy);
                new_prey.emplace_back(ox, oy, COL_PREY, PREY_HEAD_SIZE, PREY_SPEED,
                                      p.prey_vision_radius, p.prey_vision_angle, p.prey_move_cost);
            }
        }
        prey.erase(std::remove_if(prey.begin(),prey.end(),[](const Agent&a){return !a.alive;}),prey.end());
        for(auto &a : new_prey) prey.push_back(std::move(a));

        // Presas tipo 2
        std::vector<Agent> new_prey2;
        for(auto &ag : prey2){
            if(ag.alive) update_prey2_agent(ag);
            if(ag.alive && ag.energy > PREY2_REPRO_THRESHOLD && randf() < PREY2_REPRO_PROB){
                ag.energy *= 0.6f;
                float ox = ag.x + randf(-20,20);
                float oy = ag.y + randf(-20,20);
                wrap(ox,oy);
                new_prey2.emplace_back(ox, oy, COL_PREY2, PREY2_HEAD_SIZE, PREY2_SPEED,
                                       p.prey2_vision_radius, p.prey2_vision_angle, p.prey2_move_cost);
            }
        }
        prey2.erase(std::remove_if(prey2.begin(),prey2.end(),[](const Agent&a){return !a.alive;}),prey2.end());
        for(auto &a : new_prey2) prey2.push_back(std::move(a));

        // Depredadores
        std::vector<Agent> new_pred;
        for(auto &ag : predators){
            if(ag.alive) update_pred_agent(ag);
            if(ag.alive && ag.energy > PRED_REPRO_THRESHOLD && randf() < PRED_REPRO_PROB){
                ag.energy *= 0.55f;
                float ox = ag.x + randf(-20,20);
                float oy = ag.y + randf(-20,20);
                wrap(ox,oy);
                new_pred.emplace_back(ox, oy, COL_PRED, PRED_HEAD_SIZE, PRED_SPEED,
                                      p.pred_vision_radius, p.pred_vision_angle, p.pred_move_cost);
            }
        }
        predators.erase(std::remove_if(predators.begin(),predators.end(),[](const Agent&a){return !a.alive;}),predators.end());
        for(auto &a : new_pred) predators.push_back(std::move(a));

        step++;
        // Actualizar conteos
        n_pred = (int)predators.size();
        n_prey = (int)prey.size();
        n_prey2 = (int)prey2.size();
        n_grass = 0; for(auto &g : grass)  if(g.alive) n_grass++;
        n_grass2 = 0; for(auto &g : grass2) if(g.alive) n_grass2++;

        // Registrar historia cada RECORD_EVERY pasos
        if(step % RECORD_EVERY == 0){
            history.push_back({step, n_pred, n_prey, n_prey2, n_grass, n_grass2});
        }
    }

    void save_txt(const std::string &path){
        std::ofstream f(path);
        f << "=======================================================\n";
        f << "  EcoSim -- Reporte de simulacion\n";
        time_t t = time(nullptr);
        char buf[64]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&t));
        f << "  Generado: " << buf << "\n";
        f << "=======================================================\n\n";
        f << "-- PARAMETROS --\n";
        f << "  Iteraciones: " << step << "\n";
        f << "  p_captura:   " << p.p_catch << "\n";
        f << "  radio dep:   " << p.pred_vision_radius << "\n";
        f << "  ang dep:     " << p.pred_vision_angle << "\n";
        f << "  radio pre1:  " << p.prey_vision_radius << "\n";
        f << "  ang pre1:    " << p.prey_vision_angle << "\n";
        f << "  radio pre2:  " << p.prey2_vision_radius << "\n";
        f << "  ang pre2:    " << p.prey2_vision_angle << "\n";
        f << "  regrowth1:   " << p.grass_regrow_time << "\n";
        f << "  regrowth2:   " << p.grass2_regrow_time << "\n";
        f << "  costo dep:   " << p.pred_move_cost << "\n";
        f << "  costo pre1:  " << p.prey_move_cost << "\n";
        f << "  costo pre2:  " << p.prey2_move_cost << "\n\n";
        f << "-- ESTADO FINAL --\n";
        f << "  Depredadores: " << n_pred  << "\n";
        f << "  Presas1:      " << n_prey  << "\n";
        f << "  Presas2:      " << n_prey2 << "\n";
        f << "  Autotrofos1:  " << n_grass  << "\n";
        f << "  Autotrofos2:  " << n_grass2 << "\n";
        f.close();
    }

    void save_csv(const std::string &path){
        std::ofstream f(path);
        f << "t,pred,prey,prey2,grass,grass2\n";
        for(auto &r : history){
            f << r.t << ","
              << r.pred  << ","
              << r.prey  << ","
              << r.prey2 << ","
              << r.grass << ","
              << r.grass2 << "\n";
        }
        f.close();
    }
};

// ===========================================================================
// SLIDER
// ===========================================================================

struct SliderDef {
    const char *label;
    float *value;
    float  vmin, vmax;
    char   fmt; // 'p','i','f','g'
    char   cat; // 'P'=pred, 'p'=prey, 'n'=neu
};

struct UISlider {
    SliderDef def;
    int  rx, ry, rw, rh; // rect de la pista
    bool dragging = false;

    float thumb_x() const {
        float t = (*def.value - def.vmin) / (def.vmax - def.vmin);
        return rx + t * rw;
    }
    void set_from_px(int px){
        float t = (float)(px - rx) / rw;
        t = std::max(0.f, std::min(1.f, t));
        float raw = def.vmin + t*(def.vmax - def.vmin);
        if(def.fmt=='i'||def.fmt=='g') raw = (float)(int)std::round(raw);
        else raw = std::round(raw*1000.f)/1000.f;
        *def.value = raw;
    }

    Fl_Color track_color() const {
        if(def.cat=='P') return COL_PRED;
        if(def.cat=='p') return COL_PREY;
        return COL_WHITE;
    }

    void draw_slider() const {
        Fl_Color col = track_color();
        float tx = thumb_x();

        // Label izquierda + valor derecha
        fl_color(col);
        fl_font(FL_COURIER, 10);
        fl_draw(def.label, rx, ry-3);

        char vbuf[32];
        float v = *def.value;
        if(def.fmt=='p') snprintf(vbuf,sizeof(vbuf),"%.0f%%",v*100.f);
        else if(def.fmt=='g') snprintf(vbuf,sizeof(vbuf),"%d deg",(int)v);
        else if(def.fmt=='i') snprintf(vbuf,sizeof(vbuf),"%d",(int)v);
        else snprintf(vbuf,sizeof(vbuf),"%.2f",v);

        fl_color(COL_WHITE);
        int vw = (int)fl_width(vbuf);
        fl_draw(vbuf, rx+rw-vw, ry-3);

        // Pista fondo
        fl_color(fl_rgb_color(40,42,65));
        fl_rectf(rx, ry, rw, rh);

        // Relleno activo
        fl_color(col);
        fl_rectf(rx, ry, (int)(tx - rx), rh);

        // Pulgar
        fl_color(COL_WHITE);
        fl_pie((int)tx-7, ry+rh/2-7, 14, 14, 0, 360);
        fl_color(col);
        fl_pie((int)tx-5, ry+rh/2-5, 10, 10, 0, 360);
    }

    bool handle_event(int event, int ex, int ey, int panel_ox, int panel_oy){
        int ax = ex - panel_ox;
        int ay = ey - panel_oy;
        if(event == FL_PUSH){
            if(ax >= rx-2 && ax <= rx+rw+2 && ay >= ry-9 && ay <= ry+rh+9){
                dragging = true;
                set_from_px(ax);
                return true;
            }
        }
        if(event == FL_RELEASE) dragging = false;
        if(event == FL_DRAG && dragging){
            set_from_px(ax);
            return true;
        }
        return false;
    }
};

// ===========================================================================
// WIDGET DE SIMULACION (FLTK)
// ===========================================================================

class SimWidget : public Fl_Widget {
public:
    Sim sim;
    std::vector<UISlider> sliders;
    int   _save_timer = 0;
    char  _save_msg[128] = "";
    bool  _finished = false;

    SimWidget(int x, int y, int w, int h)
        : Fl_Widget(x,y,w,h)
    {
        init_colors();
        build_sliders();
        sim.reset();
    }

    void build_sliders(){
        SimParams &p = sim.p;
        // (label, ptr, vmin, vmax, fmt, cat)
        SliderDef defs[] = {
            {"p_captura",    &p.p_catch,             0.05f,1.f,  'p','n'},
            {"radio dep.",   &p.pred_vision_radius,  40.f, 250.f,'i','P'},
            {"angulo dep.",  &p.pred_vision_angle,   10.f, 360.f,'g','P'},
            {"radio presa1", &p.prey_vision_radius,  30.f, 200.f,'i','p'},
            {"ang. presa1",  &p.prey_vision_angle,   10.f, 360.f,'g','p'},
            {"radio presa2", &p.prey2_vision_radius, 30.f, 200.f,'i','p'},
            {"ang. presa2",  &p.prey2_vision_angle,  10.f, 360.f,'g','p'},
            {"regrowth1",    &p.grass_regrow_time,   50.f, 400.f,'i','n'},
            {"regrowth2",    &p.grass2_regrow_time,  50.f, 400.f,'i','n'},
            {"costo dep.",   &p.pred_move_cost,      0.05f,0.8f, 'f','P'},
            {"costo presa1", &p.prey_move_cost,      0.05f,0.6f, 'f','p'},
            {"costo presa2", &p.prey2_move_cost,     0.05f,0.6f, 'f','p'},
        };
        int n = (int)(sizeof(defs)/sizeof(defs[0]));
        int x0 = 10;
        int sw = PANEL_W - 20;
        int sh = 8;
        int gap= 32;
        int y0 = 110;  // ajustado en draw dinámicamente
        for(int i=0;i<n;i++){
            UISlider sl;
            sl.def = defs[i];
            sl.rx = x0; sl.ry = y0 + i*gap + 14;
            sl.rw = sw; sl.rh = sh;
            sliders.push_back(sl);
        }
    }

    // Reposiciona sliders entre y_top e y_bot
    void layout_sliders(int y_top, int y_bot){
        int n = (int)sliders.size();
        int total = n * 32;
        int x0 = 10, sw = PANEL_W-20;
        int y0 = (total < (y_bot-y_top))
                    ? y_top + (y_bot-y_top-total)/2 - 5
                    : y_top;
        for(int i=0;i<n;i++){
            sliders[i].rx = x0;
            sliders[i].ry = y0 + i*32 + 14;
            sliders[i].rw = sw;
            sliders[i].rh = 8;
        }
    }

    // Dibuja la grafica de poblacion en la franja inferior del panel
    void draw_population_graph(int px, int py) {
        // Area de la grafica (coordenadas absolutas de ventana)
        int graph_y = py + SIM_H - GRAPH_H;
        int graph_x = px + 4;
        int graph_w = PANEL_W - 6;
        int graph_h = GRAPH_H - 4;

        // Separador
        fl_color(COL_GRID);
        fl_line(px+8, graph_y - 13, px+PANEL_W-8, graph_y - 13);

        // Cabecera "-- Poblacion --"
        fl_color(fl_rgb_color(160,160,200));
        fl_font(FL_COURIER, 11);
        const char *hdr = "-- Población --";
        int hw = (int)fl_width(hdr);
        fl_draw(hdr, px + PANEL_W/2 - hw/2, graph_y - 2);

        // Fondo del grafico
        fl_color(fl_rgb_color(10, 12, 24));
        fl_rectf(graph_x, graph_y, graph_w, graph_h);
        fl_color(COL_GRID);
        fl_rect(graph_x, graph_y, graph_w, graph_h);

        const auto &hist = sim.history;
        if(hist.size() < 2) return;

        // Maximos para escalar
        int max_agents = 1;
        int max_grass  = 1;
        for(auto &r : hist){
            max_agents = std::max(max_agents, std::max({r.pred, r.prey, r.prey2}));
            max_grass  = std::max(max_grass,  std::max(r.grass, r.grass2));
        }
        // Escalar agentes y hierba al mismo eje (hierba suele ser mayor)
        int global_max = std::max(max_agents, max_grass);

        int n = (int)hist.size();
        // Dibujar cada serie
        struct Serie { Fl_Color col; int PopRecord::*field; };
        Serie series[] = {
            {COL_PRED, &PopRecord::pred},
            {COL_PREY, &PopRecord::prey},
            {COL_PREY2, &PopRecord::prey2},
            {COL_GRASS, &PopRecord::grass},
            {COL_GRASS2, &PopRecord::grass2},
        };
        for(auto &s : series){
            fl_color(s.col);
            fl_begin_line();
            for(int i = 0; i < n; i++){
                float fx = graph_x + (float)i / (n-1) * (graph_w - 1);
                float fy = graph_y + graph_h - 1
                           - (float)(hist[i].*s.field) / global_max * (graph_h - 2);
                fl_vertex(fx, fy);
            }
            fl_end_line();
        }
    }

    void draw() override {
        // ---------------------------------------------------------------
        // 1. AREA DE SIMULACION
        // ---------------------------------------------------------------
        fl_push_clip(x(), y(), SIM_W, SIM_H);

        // Fondo
        fl_color(COL_BG);
        fl_rectf(x(), y(), SIM_W, SIM_H);

        // Cuadricula
        fl_color(COL_GRID);
        for(int gx=0; gx<SIM_W; gx+=40)
            fl_line(x()+gx, y(), x()+gx, y()+SIM_H);
        for(int gy=0; gy<SIM_H; gy+=40)
            fl_line(x(), y()+gy, x()+SIM_W, y()+gy);

        // Conos de visión
        if(sim.show_vision){
            for(auto &ag : sim.prey) ag.draw_vision_cone();
            for(auto &ag : sim.prey2) ag.draw_vision_cone();
            for(auto &ag : sim.predators) ag.draw_vision_cone();
        }

        // Hierba
        for(auto &g : sim.grass) g.draw();
        for(auto &g : sim.grass2) g.draw();

        // Agentes
        for(auto &ag : sim.prey) ag.draw_agent();
        for(auto &ag : sim.prey2) ag.draw_agent();
        for(auto &ag : sim.predators) ag.draw_agent();

        // Overlay pausa
        if(sim.paused){
            fl_color(COL_WHITE);
            fl_font(FL_COURIER_BOLD, 13);
            const char *msg = "  PAUSADO  ";
            int mw = (int)fl_width(msg);
            int mx = x() + SIM_W/2 - mw/2 - 5;
            int my = y() + SIM_H/2 - 8;
            fl_color(FL_BLACK);
            fl_rectf(mx, my, mw+10, 20);
            fl_color(COL_WHITE);
            fl_rect(mx, my, mw+10, 20);
            fl_draw(msg, mx+5, my+14);
        }

        // Mensaje de guardado
        if(_save_timer > 0){
            fl_color(fl_rgb_color(100,240,120));
            fl_font(FL_COURIER, 11);
            fl_draw(_save_msg, x()+10, y()+SIM_H-8);
        }

        fl_pop_clip();

        // ---------------------------------------------------------------
        // 2. PANEL LATERAL
        // ---------------------------------------------------------------
        int px = x() + SIM_W;
        fl_push_clip(px, y(), PANEL_W, SIM_H);

        fl_color(COL_UI_BG);
        fl_rectf(px, y(), PANEL_W, SIM_H);
        fl_color(COL_GRID);
        fl_line(px, y(), px, y()+SIM_H);

        fl_font(FL_COURIER_BOLD, 13);
        fl_color(COL_WHITE);
        const char *title = "EcoSim PixelArt";
        int tw = (int)fl_width(title);
        fl_draw(title, px + PANEL_W/2 - tw/2, y()+18);
        fl_color(COL_GRID);
        fl_line(px+8, y()+22, px+PANEL_W-8, y()+22);

        // Contadores (columna izquierda)
        fl_font(FL_COURIER, 11);
        struct { const char *lbl; int n; Fl_Color col; } counts[] = {
            {"Dep:",  sim.n_pred, COL_PRED},
            {"Pre1:", sim.n_prey, COL_PREY},
            {"Pre2:", sim.n_prey2, COL_PREY2},
            {"Aut1:", sim.n_grass, COL_GRASS},
            {"Aut2:", sim.n_grass2, COL_GRASS2},
        };
        int yc = y()+36;
        for(auto &c : counts){
            char buf[32]; snprintf(buf,sizeof(buf),"%s %d",c.lbl,c.n);
            fl_color(c.col);
            fl_draw(buf, px+10, yc);
            yc += 14;
        }
        // Paso
        {
            char buf[32]; snprintf(buf,sizeof(buf),"t:   %d",sim.step);
            fl_color(COL_GRAY);
            fl_draw(buf, px+10, yc);
        }

        // Atajos (columna derecha)
        Fl_Color vcol = sim.show_vision ? fl_rgb_color(80,220,100) : fl_rgb_color(100,100,140);
        struct { const char *lbl; Fl_Color col; } shortcuts[] = {
            {"[R] Reiniciar", fl_rgb_color(100,100,140)},
            {"[P] Pausa", fl_rgb_color(100,100,140)},
            {"[V] Vision", vcol},
            {"[S] Guardar", fl_rgb_color(100,160,240)},
            {"[Esc] Salir", fl_rgb_color(100,100,140)},
        };
        int ys = y()+36;
        for(auto &s : shortcuts){
            fl_color(s.col);
            int sw2 = (int)fl_width(s.lbl);
            fl_draw(s.lbl, px+PANEL_W-sw2-10, ys);
            ys += 14;
        }

        // Separador
        int y_sep = y()+36 + 6*14 + 2;
        fl_color(COL_GRID);
        fl_line(px+8, y_sep, px+PANEL_W-8, y_sep);

        // Cabecera parámetros
        fl_color(fl_rgb_color(160,160,200));
        fl_font(FL_COURIER, 11);
        const char *ph = "-- Parámetros --";
        int phw = (int)fl_width(ph);
        fl_draw(ph, px + PANEL_W/2 - phw/2, y_sep+12);

        // Layout y dibujo de sliders: se reserva GRAPH_H + 16px para la grafica inferior
        layout_sliders(y_sep+16, y()+SIM_H - GRAPH_H - 16);
        for(auto &sl : sliders){
            UISlider sl2 = sl;
            sl2.rx += px;
            sl2.ry += y();
            sl2.draw_slider();
        }

        // Grafica de poblacion (franja inferior del panel)
        draw_population_graph(px, y());

        fl_pop_clip();
    }

    void tick(){
        if(_finished) return;
        sim.update();
        if(_save_timer > 0) _save_timer--;
        if(T_F > 0 && sim.step >= T_F && !_finished){
            _finished = true;
            sim.paused = true;
            sim.save_txt("output/results.txt");
            sim.save_csv("output/results.csv");
            // system("gnuplot plot_ecosim.gnu"); // Permite graficar sin usar Makefile (actualmente lo uso)
            snprintf(_save_msg, sizeof(_save_msg), "Auto-guardado: results.txt / results.csv");
            _save_timer = 300;
        }
        redraw();
    }

    int handle(int event) override {
        switch(event){
        case FL_FOCUS:
        case FL_UNFOCUS:
            return 1; // aceptar foco de teclado

        case FL_ENTER:
            take_focus();
            return 1;

        case FL_KEYDOWN: {
            int k = Fl::event_key();
            if(k == FL_Escape){ exit(0); return 1; }
            if(k=='r'||k=='R'){ sim.reset(); _finished=false; redraw(); return 1; }
            if(k=='p'||k=='P'){ sim.paused = !sim.paused; redraw(); return 1; }
            if(k=='v'||k=='V'){ sim.show_vision = !sim.show_vision; redraw(); return 1; }
            if(k=='s'||k=='S'){
                sim.save_txt("output/ecosim_reporte.txt");
                snprintf(_save_msg,sizeof(_save_msg),"Guardado: ecosim_reporte.txt");
                _save_timer = 180;
                redraw(); return 1;
            }
            return 0;
        }

        case FL_PUSH: {
            take_focus();
            int ex = Fl::event_x(), ey = Fl::event_y();
            for(auto &sl : sliders)
                if(sl.handle_event(event, ex, ey, x()+SIM_W, y())){ redraw(); return 1; }
            return 1;
        }

        case FL_RELEASE:
        case FL_DRAG: {
            int ex = Fl::event_x(), ey = Fl::event_y();
            for(auto &sl : sliders)
                if(sl.handle_event(event, ex, ey, x()+SIM_W, y())){ redraw(); return 1; }
            return 1;
        }

        default:
            return Fl_Widget::handle(event);
        }
    }
};

// ===========================================================================
// Timer callback
// ===========================================================================

static SimWidget *g_widget = nullptr;

static void timer_cb(void*){
    if(g_widget) g_widget->tick();
    Fl::repeat_timeout(1.0/FPS_TARGET, timer_cb);
}

// ===========================================================================
// MAIN
// ===========================================================================

int main(){
    Fl_Double_Window win(WIN_W, WIN_H, "EcoSim -- Modelo basado en agentes");
    win.color(fl_rgb_color(12,14,28));

    SimWidget widget(0, 0, WIN_W, WIN_H);
    g_widget = &widget;

    // win.resizable(&widget); Permite que la ventana se estire
    win.resizable(nullptr); // Garantiza que nada en la ventana se estire
    win.size_range(WIN_W, WIN_H, WIN_W, WIN_H); // Bloquea los controles del SO

    win.end();
    win.show();
    widget.take_focus();

    Fl::add_timeout(1.0/FPS_TARGET, timer_cb);
    return Fl::run();
}
