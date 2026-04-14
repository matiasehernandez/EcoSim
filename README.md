# EcoSim — Modelo de simulación basado en agentes

Simulación ecológica interactiva implementada en **C++ / FLTK** que modela la dinámica de poblaciones entre autótrofos, presas y depredadores en un entorno toroidal con física de visión en cono.

![EcoSim en acción.](images/ecosim_demo.gif)

## Autor
- Matías Ezequiel Hernández Rodríguez
- Email: matiasehernandez@gmail.com

## Características

- **Tres niveles tróficos**: Depredadores, dos tipos de presas y dos tipos de autótrofos (hierba) con comportamientos diferenciados.
- **Visión en cono**: cada agente percibe su entorno dentro de un ángulo y radio configurables; las presas huyen y los depredadores persiguen solo lo que pueden "ver".
- **Energía y reproducción**: los agentes gastan energía al moverse y se reproducen estocásticamente al superar un umbral.
- **Parámetros ajustables en tiempo real**: sliders integrados en el panel lateral permiten modificar radio de visión, ángulo, costo de movimiento, probabilidad de captura y tiempo de rebrote de la hierba sin reiniciar la simulación.
- **Registro automático de métricas**: al finalizar las `T_F` iteraciones, la simulación se pausa y guarda `results.txt` y `results.csv` en `output/`.
- **Gráficas con Gnuplot**: el script `plot_ecosim.gnu` genera automáticamente la dinámica temporal y cinco planos de fase en formato PNG.
- **Interfaz pixel-art**: paleta oscura con sprites segmentados, cuadrícula y visualización opcional de los conos de visión.

## Requisitos

| Herramienta | Versión mínima recomendada |
|---|---|
| g++ | C++17 |
| [FLTK](https://www.fltk.org/) | 1.3.x |
| Gnuplot | 5.x |
| Make | cualquier versión |

En Debian/Ubuntu:

```bash
sudo apt install libfltk1.3-dev gnuplot build-essential
```


## Compilación y ejecución

```bash
# Compilar
make

# Ejecutar (y graficar al terminar)
make run

# Solo graficar (requiere output/results.csv)
make plot

# Limpiar el ejecutable
make clean
```

O manualmente:

```bash
g++ -O2 -o ecosim ecosim.cpp $(fltk-config --cxxflags --ldflags) -lm
./ecosim
```

## Controles

| Tecla | Acción |
|---|---|
| `R` | Reiniciar simulación |
| `P` | Pausar / reanudar |
| `V` | Mostrar / ocultar conos de visión |
| `S` | Guardar métricas manualmente (`output/ecosim_reporte.txt`) |
| `Esc` | Salir |

Los **sliders** del panel lateral modifican parámetros en tiempo real sin necesidad de reiniciar.

## Estructura del proyecto

```
ecosim/
|-- ecosim.cpp          # Código fuente principal
|-- Makefile            # Reglas de compilación y ejecución
|-- plot_ecosim.gnu     # Script Gnuplot para graficar resultados
|-- output/             # Directorio de salida (generado en ejecución)
    |-- results.csv         # Serie temporal de poblaciones
    |-- results.txt         # Reporte en texto plano
    |-- results.png         # Dinámica temporal (gráfica)
    |-- results_phase_dep_pre.png
    |-- results_phase_dep_pre2.png
    |-- results_phase_pre_aut.png
    |-- results_phase_pre2_aut2.png
    |-- results_phase_dep_aut.png
```

El directorio `output/` debe existir antes de ejecutar. Créalo con `mkdir -p output`.

## Parámetros principales

Los valores por defecto se definen como constantes en la cabecera de `ecosim.cpp` y pueden modificarse antes de compilar.

| Parámetro | Valor por defecto | Descripción |
|---|---|---|
| `N_PREDATORS` | 6 | Depredadores iniciales |
| `N_PREY / N_PREY2` | 15 / 15 | Presas iniciales (tipo 1 / tipo 2) |
| `N_GRASS / N_GRASS2` | 200 / 150 | Autótrofos iniciales |
| `T_F` | 25 000 | Iteraciones totales (`-1` = indefinido) |
| `FPS_TARGET` | 60 | Fotogramas por segundo objetivo |
| `RANDOM_SEED` | 42 | Semilla para reproducibilidad |
| `P_CATCH` | 0.23 | Probabilidad de captura exitosa |
| `PRED_VISION_RADIUS` | 118 px | Radio de visión del depredador |
| `PREY_VISION_ANGLE` | 124° | Ángulo de visión de la presa 1 |
| `GRASS_REGROW_TIME` | 180 pasos | Tiempo de rebrote del autótrofo 1 |


## Salidas generadas

Tras completar `T_F` pasos (o al presionar `S`), la simulación exporta:

- **`results.csv`**: columnas `t, depredador, presa1, presa2, autotrofo1, autotrofo2`, una fila cada 10 pasos.
- **`results.txt`**: resumen legible con los mismos datos.
- **PNGs vía Gnuplot**: dinámica temporal + planos de fase de todos los pares relevantes de especies (coloreados por tiempo normalizado con paleta Viridis).

## Licencia

Este proyecto está bajo la licencia MIT. Ver el archivo LICENSE para más detalles.
