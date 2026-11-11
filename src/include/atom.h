#ifndef ATOM_H
#define ATOM_H

#define ATOM_POINTS 400
#define BOHR_RADIUS 1.0f
#define ATOM_PI       3.14159265358979323846f
#define ATOM_TWO_PI   6.28318530717958647692f
#define ATOM_HALF_PI  1.57079632679489661923f
#define PROTON_POINTS 80
#define PHYSICS_PROTON_RADIUS 0.00006f
#define DISPLAY_PROTON_RADIUS 0.12f
#define NEUTRON_POINTS 80
#define ATOM_LENGHT_UNIT 1.0f
#define ATOM_MASS_ELECTRON 1.0f
#define MASS_PROTON 1836.152673f
#define MASS_NEUTRON 1838.683661f
#define ATOM_GRAVITY_G    1.2103e-43f
#define GRAVITY_RENDER_SCALE 1.0e30f
#define COULOMB_RENDER_SCALE 1.0f
#define MASS_ELECTRON 1.0f
#define ELECTRON_COUNT 26


#include <stdint.h>
#include <kernel_api.h>

typedef struct {
    float x;
    float y;
    float z;

    uint32_t color;
    float charge;

    float vx;
    float vy;
    float vz;

    float mass;

    float spin;
    int orbital_n;
    int orbital_l;
    int orbital_m;
    
} AtomPoint;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    int n;
    int l;
    int m;
} HydrogenOrbital;

typedef struct {
    AtomPoint particle;

    int orbital_n;
    int orbital_l;
    int orbital_m;
    int spin;
} Electron;

extern uint32_t atom_rand_state;


uint32_t atom_rand(void);
float atom_rand01(void);
float atom_sqrt(float x);
float atom_exp_neg(float x);
float hydrogen_1s_density(float r);

void atom_generate_cloud(AtomPoint *points, int count);
int project_point(Vec3 p, int *screen_x, int *screen_y, KernelAPI *api);
void atom_render(uint8_t *buffer, AtomPoint *points, int count, float angle, KernelAPI *api);

#endif