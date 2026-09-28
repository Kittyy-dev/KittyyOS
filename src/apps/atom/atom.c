#include <colors.h>
#include <stdint.h>
#include <kernel_api.h>
#include <atom.h>

uint32_t atom_rand_state = 123456789;

static AtomPoint atom_points[ATOM_POINTS];

static HydrogenOrbital orbital = {1, 0, 0};

static AtomPoint proton_points[PROTON_POINTS];

static AtomPoint neutron_points[NEUTRON_POINTS];

float atom_sqrt(float x) {
    if (x <= 0.0f) {
        return 0.0f;
    }

    float r = x;

    for (int i = 0; i < 8; i++) {
        r = 0.5f * (r + x / r);
    }

    return r;
}

uint32_t atom_rand(void) {
    atom_rand_state = atom_rand_state * 1664525u + 1013904223u;

    return atom_rand_state;
}

float atom_rand01(void) {
    return (float)(atom_rand() & 0xFFFFFF) / 16777215.0f;
}

float atom_sin(float x) {
    while (x > ATOM_PI) {
        x -= ATOM_TWO_PI;
    }

    while (x < - ATOM_PI) {
        x += ATOM_TWO_PI;
    }

    int sign = 1;

    if (x > ATOM_HALF_PI) {
        x = ATOM_PI - x;
    } else if (x < - ATOM_HALF_PI) {
        x = - ATOM_PI - x;
    }

    if (x < 0.0f) {
        sign = -1;
        x = - x;
    }

    float x2 = x * x;

    float result = x - (x * x2) / 6.0f + (x * x2 * x2) / 120.0f - (x * x2 * x2 * x2) / 5040.0f;

    return result * (float)sign;
}

float atom_cos(float x) {
    return atom_sin(x + ATOM_HALF_PI);
}

void atom_clear_buffer(uint8_t *buffer, uint32_t color, KernelAPI *api) {
    for (int y = 0; y < api->vbe->height; y++) {
        for (int x = 0; x < api->vbe->width; x++) {
            api->atom_putpixel(buffer, x, y, color);
        }
    }
}

Vec3 atom_rotate_y(Vec3 p, float angle) {
    Vec3 r;

    float s = atom_sin(angle);
    float c = atom_cos(angle);

    r.x = p.x * c + p.z * s;
    r.y = p.y;
    r.z = - p.x * s + p.z * c;

    return r;
}

uint32_t atom_charge_color(int charge) {
    if (charge > 0) {
        return 0xFF4040;
    }

    if (charge < 0) {
        return 0x4080FF;
    }

    return 0xFFFFFF;
}

void atom_generate_cloud(AtomPoint *points, int count) {
    for (int i = 0; i < count; i++) {

        float x, y, z;

        do {
            x = atom_rand01() * 2.0f - 1.0f;
            y = atom_rand01() * 2.0f - 1.0f;
            z = atom_rand01() * 2.0f - 1.0f;
        } while (x*x + y*y + z*z > 1.0f);

        points[i].x = x;
        points[i].y = y;
        points[i].z = z;

        points[i].charge = -1.0f;
        points[i].color = 0x80C8FF;
    }
}

int project_point(Vec3 p, int *screen_x, int *screen_y, KernelAPI *api) {
    float camera_z = 3.0f;

    float z = p.z + camera_z;

    if (z <= 0.1f) {
        return 0;
    }

    float scale = 300.0f / z;

    *screen_x = (int)(api->vbe->width / 2 + p.x * scale);
    *screen_y = (int)(api->vbe->height / 2 - p.y * scale);

    return 1;
}

void atom_generate_proton(AtomPoint *points, int count) {
    for (int i = 0; i < count; i++) {

        float x;
        float y;
        float z;
        float r;

        do {
            x = atom_rand01() * 2.0f - 1.0f;
            y = atom_rand01() * 2.0f - 1.0f;
            z = atom_rand01() * 2.0f - 1.0f;

            r = atom_sqrt(x*x + y*y + z*z);

        } while (r > 1.0f || r < 0.0001f);

        float radius = r * DISPLAY_PROTON_RADIUS;

        points[i].x = (x / r) * radius;
        points[i].y = (y / r) * radius;
        points[i].z = (z / r) * radius;

        points[i].charge = +1.0f;
        points[i].mass = MASS_PROTON;
        points[i].color = atom_charge_color(+1);
    }
}

void atom_render_proton(uint8_t *buffer, AtomPoint *points, int count, float angle, KernelAPI *api) {
    for (int i = 0; i < count; i++) {
        Vec3 p = {points[i].x, points[i].y, points[i].z};

        p = atom_rotate_y(p, angle);

        int sx;
        int sy;

        if (!project_point(p, &sx, &sy, api)) {
            continue;
        }

        if (sx < 0 || sy < 0 || sx >= api->vbe->width || sy >= api->vbe->height) {
            continue;
        }

        api->atom_putpixel(buffer, sx, sy, points[i].color);
    }
}

void atom_generate_neutron(AtomPoint *points, int count) {
    for (int i = 0; i < count; i++) {
        float x, y, z;
        float r;

        do {
            x = atom_rand01() * 2.0f - 1.0f;
            y = atom_rand01() * 2.0f - 1.0f;
            z = atom_rand01() * 2.0f - 1.0f;

            r = atom_sqrt(x * x + y * y + z * z);
        } while (r > 1.0f || r < 0.0001f);

        float  radius = r * DISPLAY_PROTON_RADIUS;

        points[i].x = (x / r) * radius;
        points[i].y = (y / r) * radius;
        points[i].z = (z / r) * radius;

        points[i].charge = 0.0f;
        points[i].mass = MASS_NEUTRON;
        points[i].color = atom_charge_color(0);
    }
}

void atom_render(uint8_t *buffer, AtomPoint *points, int count, float angle, KernelAPI *api) {
    for (int i = 0; i < count; i++) {

        Vec3 p = {points[i].x, points[i].y, points[i].z};

        p = atom_rotate_y(p, angle);

        int sx;
        int sy;

        if (!project_point(p, &sx, &sy, api)) {
            continue;
        }

        if (sx < 0 || sy < 0 || sx >= api->vbe->width || sy >= api->vbe->height) {
            continue;
        }

        api->atom_putpixel(buffer, sx, sy, points[i].color);
    }
}

void atom_render_neutron(uint8_t *buffer, AtomPoint *points, int count, float angle, KernelAPI *api) {
    for (int i = 0; i < count; i++) {
        Vec3 p = {points[i].x, points[i].y, points[i].z};

        p = atom_rotate_y(p, angle);

        int sx, sy;

        if (!project_point(p, &sx, &sy, api)) {
            continue;
        }

        if (sx < 0 || sy < 0 || sx >= api->vbe->width || sy >= api->vbe->height) {
            continue;
        }

        api->atom_putpixel(buffer, sx, sy, points[i].color);
    }
}

float atom_exp_neg(float x) {
    if (x <= 0.0f) {
        return 1.0f;
    }

    if (x >= 16.0f) {
        return 0.0f;
    }

    float y = x / 16.0f;

    float e = 1.0f - y + (y * y) * 0.5f - (y * y * y) / 6.0f + (y * y * y * y) / 24.0f - (y * y * y * y * y) / 120.0f;

    e = e * e;
    e = e * e;
    e = e * e;
    e = e * e;

    return e;
}

float hydrogen_1s_density(float r) {
    return (1.0f / ATOM_PI) * atom_exp_neg(2.0f * r);
}

float hydrogen_1s_sample_radius(void) {
    const float R_MAX = 8.0f;
    const float P_MAX = 0.54134f;

    while (1) {
        float r = atom_rand01() * R_MAX;

        float p = 4.0f * r * r * atom_exp_neg(2.0f * r);

        float test = atom_rand01() * P_MAX;

        if (test <= p) {
            return r;
        }
    }
}

float gravity_force(float m1, float m2, float distance) {
    if (distance <= 0.000001f) {
        return 0.0f;
    }

    return ATOM_GRAVITY_G * m1 * m2 / (distance * distance);
}

Vec3 gravity_acceleration(Vec3 position, Vec3 source_position, float source_mass) {
    Vec3 result;

    float dx = source_position.x - position.x;
    float dy = source_position.y - position.y;
    float dz = source_position.z - position.z;

    float r2 = dx * dx + dy * dy + dz * dz;

    if (r2 <= 0.000001f) {
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
        return result;
    }

    float r = atom_sqrt(r2);

    float factor = ATOM_GRAVITY_G * source_mass / (r2 * r);

    result.x = dx * factor;
    result.y = dy * factor;
    result.z = dz * factor;

    return result;
}

void atom_render_gravity(uint8_t *buffer, Vec3 position, Vec3 acceleration, float scale, KernelAPI *api) {
    Vec3 end;

    end.x = position.x + acceleration.x * scale;
    end.y = position.y + acceleration.y * scale;
    end.z = position.z + acceleration.z * scale;

    int x1, y1;
    int x2, y2;

    if (!project_point(position, &x1, &y1, api)) {
        return;
    }

    if (!project_point(end, &x2, &y2, api)) {
        return;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = dx < 0 ? - dx : dx;

    if ((dy < 0 ? - dy : dy) > steps) {
        steps = dy < 0 ? - dy : dy;
    }

    if (steps == 0) {
        return;
    }

    for (int i = 0; i <= steps; i++) {
        int x = x1 + dx * i / steps;
        int y = y1 + dy * i / steps;

        if (x >= 0 && y >= 0 && x < api->vbe->width && y < api->vbe->height) {
            api->atom_putpixel(buffer, x, y, 0xFFFFFF);
        }
    }
}

void atom_generate_hydrogen(AtomPoint *points, int count) {
    for (int i = 0; i < count; i++) {
        float x;
        float y;
        float z;

        do {
            x = atom_rand01() * 2.0f - 1.0f;
            y = atom_rand01() * 2.0f - 1.0f;
            z = atom_rand01() * 2.0f - 1.0f;
        } while (x * x + y * y + z * z > 1.0f || x * x + y * y + z * z < 0.0001f);

        float length = atom_sqrt(x * x + y * y + z * z);

        x /= length;
        y /= length;
        z /= length;

        float radius = hydrogen_1s_sample_radius();

        points[i].x = x * radius;
        points[i].y = y * radius;
        points[i].z = z * radius;

        float speed = 1.0f / atom_sqrt(radius);

        points[i].vx = - y * speed;
        points[i].vy = x * speed;
        points[i].vz = 0.0f;

        points[i].charge = -1.0f;
        points[i].mass = MASS_ELECTRON;

        float density = hydrogen_1s_density(radius);

        if (density > 0.20f) {
            points[i].color = 0x4080FF;
        } else if (density > 0.05f) {
            points[i].color = 0x3060C0;
        } else {
            points[i].color = 0x203060;
        }

        // points[i].color = atom_charge_color(-1);
    }
}

void atom_present(uint8_t *buffer, KernelAPI *api) {
    uint8_t *framebuffer = (uint8_t*)(uintptr_t)api->vbe->physbase;

    uint64_t size = (uint64_t)api->vbe->pitch * api->vbe->height;

    for (uint64_t i = 0; i < size; i++) {
        framebuffer[i] = buffer[i];
    }
}

Vec3 coulomb_force(Vec3 position, float charge, Vec3 source_position, float source_charge) {
    Vec3 result;

    float dx = source_position.x - position.x;
    float dy = source_position.y - position.y;
    float dz = source_position.z - position.z;

    float r2 = dx * dx + dy * dy + dz * dz;

    if (r2 <= 0.000001f) {
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
        return result;
    }

    float r = atom_sqrt(r2);

    float factor = charge * source_charge / (r2 * r);

    result.x = dx * factor;
    result.y = dy * factor;
    result.z = dz * factor;

    return result;
}

void atom_apply_force(AtomPoint *a, AtomPoint *b, float dt) {
    Vec3 position = {a->x, a->y, a->z};
    Vec3 source_position = {b->x, b->y, b->z};
    Vec3 electric = coulomb_force(position, a->charge, source_position, b->charge);
    Vec3 gravity = gravity_acceleration(position, source_position, b->mass);

    float inv_mass = 1.0f / a->mass;

    a->vx += (electric.x * inv_mass + gravity.x) * dt;
    a->vy += (electric.y * inv_mass + gravity.y) * dt;
    a->vz += (electric.z * inv_mass + gravity.z) * dt;
}

void atom_update(float dt) {
    for (int i = 0; i < ATOM_POINTS; i++) {
        for (int j = 0; j < PROTON_POINTS; j++) {
            atom_apply_force(&atom_points[i], &proton_points[j], dt);
        }

        for (int j = 0; j < NEUTRON_POINTS; j++) {
            atom_apply_force(&atom_points[i], &neutron_points[j], dt);
        }

        atom_points[i].x += atom_points[i].vx * dt;
        atom_points[i].y += atom_points[i].vy * dt;
        atom_points[i].z += atom_points[i].vz * dt;
    }

    for (int i = 0; i < PROTON_POINTS; i++) {
        for (int j = 0; j < ATOM_POINTS; j++) {
            atom_apply_force(&proton_points[i], &atom_points[j], dt);
        }

        for (int j = 0; j < NEUTRON_POINTS; j++) {
            atom_apply_force(&proton_points[i], &neutron_points[j], dt);
        }

        proton_points[i].x += proton_points[i].vx * dt;
        proton_points[i].y += proton_points[i].vy * dt;
        proton_points[i].z += proton_points[i].vz * dt;
    }

    for (int i = 0; i < NEUTRON_POINTS; i++) {

        for (int j = 0; j < ATOM_POINTS; j++) {
            atom_apply_force(&neutron_points[i], &atom_points[j], dt);
        }

        for (int j = 0; j < PROTON_POINTS; j++) {
            atom_apply_force(&neutron_points[i], &proton_points[j], dt);
        }

        neutron_points[i].x += neutron_points[i].vx * dt;
        neutron_points[i].y += neutron_points[i].vy * dt;
        neutron_points[i].z += neutron_points[i].vz * dt;
    }
}

void atom_render_vector(uint8_t *buffer, Vec3 position, Vec3 vector, float scale, uint32_t color, KernelAPI *api) {
    Vec3 end;

    end.x = position.x + vector.x * scale;
    end.y = position.y + vector.y * scale;
    end.z = position.z + vector.z * scale;

    int x1, y1;
    int x2, y2;

    if (!project_point(position, &x1, &y1, api)) {
        return;
    }

    if (!project_point(end, &x2, &y2, api)) {
        return;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = dx < 0 ? - dx : dx;

    if ((dy < 0 ? - dy : dy) > steps) {
        steps = dy < 0 ? - dy : dy;
    }

    if (steps == 0) {
        return;
    }

    for (int i = 0; i <= steps; i++) {
        int x = x1 + dx * i / steps;
        int y = y1 + dy * i / steps;

        if (x >= 0 && y >= 0 && x < api->vbe->width && y < api->vbe->height) {
            api->atom_putpixel(buffer, x, y, color);
        }
    }
}

void atom_make_fps_text(uint32_t fps, char *buffer)  {
    char temp[16];
    int i = 0;
    int j = 0;

    buffer[j++] = 'F';
    buffer[j++] = 'P';
    buffer[j++] = 'S';
    buffer[j++] = ':';
    buffer[j++] = ' ';
    
    if (fps == 0) {
        buffer[j++] = '0';
        buffer[j] = '\0';
        return;
    }

    while (fps > 0) {
        temp[i++] = '0' + (fps % 10);
        fps /= 10;
    }

    while (i > 0) {
        buffer[j++] = temp[--i];
    }

    buffer[j] = '\0';
}

void atom_test(KernelAPI *api) {
    atom_generate_hydrogen(atom_points, ATOM_POINTS);

    atom_generate_proton(proton_points, PROTON_POINTS);

    atom_generate_neutron(neutron_points, NEUTRON_POINTS);

    uint64_t buffer_size = (uint64_t)api->vbe->pitch * api->vbe->height;

    uint8_t *backbuffer = api->kmalloc(buffer_size);

    uint64_t fps_start = api->rdtsc();
    uint32_t fps_frames = 0;
    uint32_t fps = 0;

    if (backbuffer == NULL) {
        api->kprintf("ERROR: No Backbuffer!\n");
        return;
    }

    float angle = 0.0f;

    while (1) {
        atom_clear_buffer(backbuffer, 0x000000, api);

        // atom_update(0.001f);
        atom_render(backbuffer, atom_points, ATOM_POINTS, angle, api);
        atom_render_proton(backbuffer, proton_points, PROTON_POINTS, angle, api);
        atom_render_neutron(backbuffer, neutron_points, NEUTRON_POINTS, angle, api);

        fps_frames++;

        uint64_t now = api->rdtsc();
        uint64_t elapsed = now - fps_start;

        if (elapsed >= api->get_tsc_freq()) {
            fps = fps_frames;
            fps_frames = 0;
            fps_start = now;
        }

        char fps_text[32];
        atom_make_fps_text(fps, fps_text);
        atom_present(backbuffer, api);

        api->vbe_print_at(0, 0, fps_text, 0xFFFFFF);

        angle += 0.02f;

        if (angle >= ATOM_TWO_PI) {
            angle -= ATOM_TWO_PI;
        }
    }
}