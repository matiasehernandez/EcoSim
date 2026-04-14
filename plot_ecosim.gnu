# ============================================================
# plot_ecosim.gnu -- Graficas de EcoSim ABM
# Uso: gnuplot plot_ecosim.gnu
#      (requiere results.csv en el mismo directorio)
# ============================================================

set datafile separator ','

datafile = "output/results.csv"

COL_PRED  = "#DC3C3C"
COL_PREY  = "#3C82DC"
COL_PREY2 = "#8C8C96"
COL_GRAS  = "#3CB450"
COL_GRAS2 = "#1A5C2A"

set palette defined ( \
    0.00 '#440154', \
    0.13 '#482878', \
    0.25 '#3e4989', \
    0.38 '#31688e', \
    0.50 '#26828e', \
    0.63 '#1f9e89', \
    0.75 '#35b779', \
    0.88 '#6ece58', \
    1.00 '#fde725' )

# ============================================================
# 1. DINAMICA TEMPORAL
# ============================================================
set terminal pngcairo enhanced font "sans,10" size 1200,525
set output "output/results.png"

set style data lines
set key top right font ",9" box opaque
set xlabel "Tiempo (iteraciones)" font ",10"
set ylabel "Población (N)"        font ",10"
set title  "Dinámica de población" font ",11"
set grid lt 1 lc rgb "#dddddd" lw 0.5
set xrange [*:*]
set yrange [0:*]
set ytics nomirror

plot \
    datafile every ::1 using 1:2 title "Depredador"    lc rgb COL_PRED  lw 1.2 with lines, \
    datafile every ::1 using 1:3 title "Presa1 (Azul)" lc rgb COL_PREY  lw 1.2 with lines, \
    datafile every ::1 using 1:4 title "Presa2 (Gris)" lc rgb COL_PREY2 lw 1.2 with lines, \
    datafile every ::1 using 1:5 title "Autótrofo1"    lc rgb COL_GRAS  lw 1.0 with lines, \
    datafile every ::1 using 1:6 title "Autótrofo2"    lc rgb COL_GRAS2 lw 1.0 with lines

# ============================================================
# 2. DIAGRAMAS DE FASE
# ============================================================
set terminal pngcairo enhanced font "sans,9" size 675,675

set colorbox vertical user origin 0.87,0.15 size 0.04,0.70
set cbrange [0:1]
set cblabel "Tiempo (t normalizado)" font ",8"
set cbtics 0.25
set key bottom right font ",8" box opaque

stats datafile every ::1 using 1 nooutput
NROWS = STATS_records

# Fase 1: Depredador vs Presa1
set output "output/results_phase_dep_pre.png"
set xlabel "Depredador (N)" font ",10"
set ylabel "Presa1 (N)"     font ",10"
set title  "Plano de fase: Depredador / Presa1" font ",10"
set autoscale xy
plot \
    datafile every ::1 using 2:3:($0/(NROWS>1?NROWS-1:1)) \
        with lines lc palette lw 0.9 notitle, \
    datafile every ::1::1 using 2:3 \
        with points pt 7 ps 1.4 lc rgb "#00aa00" title "inicio", \
    datafile every ::NROWS::NROWS using 2:3 \
        with points pt 5 ps 1.4 lc rgb "red" title "final"

# Fase 2: Depredador vs Presa2
set output "output/results_phase_dep_pre2.png"
set xlabel "Depredador (N)" font ",10"
set ylabel "Presa2 (N)"     font ",10"
set title  "Plano de fase: Depredador / Presa2" font ",10"
set autoscale xy
plot \
    datafile every ::1 using 2:4:($0/(NROWS>1?NROWS-1:1)) \
        with lines lc palette lw 0.9 notitle, \
    datafile every ::1::1 using 2:4 \
        with points pt 7 ps 1.4 lc rgb "#00aa00" title "inicio", \
    datafile every ::NROWS::NROWS using 2:4 \
        with points pt 5 ps 1.4 lc rgb "red" title "final"

# Fase 3: Presa1 vs Autotrofo1
set output "output/results_phase_pre_aut.png"
set xlabel "Presa1 (N)"     font ",10"
set ylabel "Autótrofo1 (N)" font ",10"
set title  "Plano de fase: Presa1 / Autótrofo1" font ",10"
set autoscale xy
plot \
    datafile every ::1 using 3:5:($0/(NROWS>1?NROWS-1:1)) \
        with lines lc palette lw 0.9 notitle, \
    datafile every ::1::1 using 3:5 \
        with points pt 7 ps 1.4 lc rgb "#00aa00" title "inicio", \
    datafile every ::NROWS::NROWS using 3:5 \
        with points pt 5 ps 1.4 lc rgb "red" title "final"

# Fase 4: Presa2 vs Autotrofo2
set output "output/results_phase_pre2_aut2.png"
set xlabel "Presa2 (N)"     font ",10"
set ylabel "Autótrofo2 (N)" font ",10"
set title  "Plano de fase: Presa2 / Autótrfo2" font ",10"
set autoscale xy
plot \
    datafile every ::1 using 4:6:($0/(NROWS>1?NROWS-1:1)) \
        with lines lc palette lw 0.9 notitle, \
    datafile every ::1::1 using 4:6 \
        with points pt 7 ps 1.4 lc rgb "#00aa00" title "inicio", \
    datafile every ::NROWS::NROWS using 4:6 \
        with points pt 5 ps 1.4 lc rgb "red" title "final"

# Fase 5: Depredador vs Autótrofo1
set output "output/results_phase_dep_aut.png"
set xlabel "Depredador (N)" font ",10"
set ylabel "Autótrofo1 (N)" font ",10"
set title  "Plano de fase: Depredador / Autótrfo1" font ",10"
set autoscale xy
plot \
    datafile every ::1 using 2:5:($0/(NROWS>1?NROWS-1:1)) \
        with lines lc palette lw 0.9 notitle, \
    datafile every ::1::1 using 2:5 \
        with points pt 7 ps 1.4 lc rgb "#00aa00" title "inicio", \
    datafile every ::NROWS::NROWS using 2:5 \
        with points pt 5 ps 1.4 lc rgb "red" title "final"

set output
print "Graficas generadas correctamente."
