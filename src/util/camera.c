#include "camera.h"

#include <float.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "prettify_c.h"

static double current_time_secs() {
  static clock_t Start;
  static bool IsInit = false;

  clock_t now = clock();

  if (not IsInit) {
    Start = now;
    IsInit = true;
  }

  return ((double) (now - Start)) / CLOCKS_PER_SEC;
}

#define current_time current_time_secs

#define MAX_ZOOM 667

PlotCamera PlotCamera_new(float start_x, float start_y) {
  // Todo: GetTime() ?
  double now = current_time();

  PlotCamera camera = {.pos = (Vector2){start_x, start_y},
                       .vel = (Vector2){0.0f, 0.0f},
                       .vel_start_time = now,
                       .next_vel = (Vector2){0.0f, 0.0f},
                       .last_drag_time = now,
                       .vel_exp = CAMERA_VEL_EXP,
                       .vel_inertia = 0.3f,
                       .zoom = 20.0f,
                       .zoom_vel = 0.0f,
                       .zoom_vel_start_time = now,
                       .zoom_exp = CAMERA_ZOOM_EXP};

  return camera;
}
/*
void Camera_wrap_x(Camera* self, float max);
void Camera_wrap_y(Camera* self, float max);
void Camera_update_anim(Camera* self);
float Camera_zoom(const Camera* self);
float Camera_zoom_vel(const Camera* self);
void Camera_set_zoom(Camera* self, float new_zoom);
void Camera_on_zoom(Camera* self, float delta);
void Camera_on_drag(Camera* self, Vector2 drag);
void Camera_on_drag_start(Camera* self);
void Camera_on_drag_end(Camera* self);


*/

static float clamp(float x, float min, float max) {
  return x < min ? min : (x > max ? max : x);
}

Vector2 PlotCamera_pos(const PlotCamera* self) {
  double q = 1.0 / self->vel_exp;

  Vector2 pos = self->pos;
  Vector2 vel = self->vel;
  double t = current_time() - self->vel_start_time;

  float coef = (float)((1.0 - pow(q, t)) / (1.0 - q));
  Vector2 new_pos;
  new_pos.x = pos.x + self->vel_inertia * vel.x * coef;
  new_pos.y = pos.y + self->vel_inertia * vel.y * coef;

  new_pos.x = clamp(new_pos.x, -FLT_MAX, FLT_MAX);
  new_pos.y = clamp(new_pos.y, -FLT_MAX, FLT_MAX);

  return new_pos;
}

Vector2 PlotCamera_vel(const PlotCamera* self) {
  double q = 1.0 / self->vel_exp;
  double t = current_time() - self->zoom_vel_start_time;
  float coef = (float)pow(q, t);
  Vector2 new_vel;
  new_vel.x = self->vel_inertia * self->vel.x * coef;
  new_vel.y = self->vel_inertia * self->vel.y * coef;
  return new_vel;
}

void PlotCamera_set_pos(PlotCamera* self, Vector2 new_pos) {
  Vector2 new_vel = PlotCamera_vel(self);
  self->pos = new_pos;
  self->vel = new_vel;
  self->vel_start_time = current_time();
}

void PlotCamera_wrap_x(PlotCamera* self, float max) {
  Vector2 pos = PlotCamera_pos(self);
  float new_x = fmodf((fmodf(pos.x, max) + max), max);
  float dx = new_x - pos.x;
  self->pos.x += dx;
}

void PlotCamera_wrap_y(PlotCamera* self, float max) {
  Vector2 pos = PlotCamera_pos(self);
  float new_y = fmodf((fmodf(pos.y, max) + max), max);
  float dy = new_y - pos.y;
  self->pos.y += dy;
}

void PlotCamera_update_anim(PlotCamera* self) {
  Vector2 new_pos = PlotCamera_pos(self);
  Vector2 new_vel = PlotCamera_vel(self);
  float new_zoom_vel = PlotCamera_zoom_vel(self);
  double now = current_time();

  self->pos = new_pos;
  self->vel = new_vel;
  self->zoom_vel = new_zoom_vel;
  self->vel_start_time = now;
  self->zoom_vel_start_time = now;
}

float PlotCamera_zoom(const PlotCamera* self) {
  double q = 1.0 / self->zoom_exp;
  double t = current_time() - self->zoom_vel_start_time;
  return self->zoom + self->zoom_vel * ((1.0 - pow(q, t)) / (1.0 - q));
}

float PlotCamera_zoom_vel(const PlotCamera* self) {
  double q = 1.0 / self->zoom_exp;
  double t = current_time() - self->zoom_vel_start_time;
  return self->zoom_vel * (float)pow(q, t);
}

void PlotCamera_set_zoom(PlotCamera* self, float new_zoom) {
  self->zoom = new_zoom;
  self->zoom_vel = 0.0f;
  self->zoom_vel_start_time = current_time();
}

void PlotCamera_on_zoom(PlotCamera* self, float delta) {
  self->zoom = PlotCamera_zoom(self);
  self->zoom_vel = PlotCamera_zoom_vel(self) + delta;
  self->zoom_vel_start_time = current_time();
}

void PlotCamera_on_drag(PlotCamera* self, Vector2 drag) {
  self->pos.x += drag.x;
  self->pos.y += drag.y;

  self->pos.x = clamp(self->pos.x, -FLT_MAX, FLT_MAX);
  self->pos.y = clamp(self->pos.y, -FLT_MAX, FLT_MAX);

  double now = current_time();
  float dt = (float)(now - self->last_drag_time);
  if (dt < 0.001) dt = 0.001;

  self->last_drag_time = now;
  Vector2 this_drag_vel = {drag.x / dt, drag.y / dt};

  if (isnan(this_drag_vel.x)) this_drag_vel.x = 0.0;
  if (isinf(this_drag_vel.x))
    this_drag_vel.x = this_drag_vel.x < 0.0 ? -FLT_MAX : FLT_MAX;

  if (isnan(this_drag_vel.y)) this_drag_vel.y = 0.0;
  if (isinf(this_drag_vel.y))
    this_drag_vel.y = this_drag_vel.y < 0.0 ? -FLT_MAX : FLT_MAX;

  self->next_vel.x = self->next_vel.x * 0.2f + this_drag_vel.x * 0.8f;
  self->next_vel.y = self->next_vel.y * 0.2f + this_drag_vel.y * 0.8f;
}

void PlotCamera_on_drag_start(PlotCamera* self) {
  self->pos = PlotCamera_pos(self);
  self->vel = (Vector2){0.0f, 0.0f};
  self->vel_start_time = current_time();
}

#define Clamp(a, min, max)  (a) < (min) ? (min) : ((a) > (max) ? (max) : (a))
#define MaxDragTime 0.25

void PlotCamera_on_drag_end(PlotCamera* self) {
  double now = current_time();
  self->pos = PlotCamera_pos(self);
  double drag_coef = Clamp(MaxDragTime - (now - self->last_drag_time), 0.0, MaxDragTime) / MaxDragTime;
  self->vel.x = self->next_vel.x * drag_coef;
  self->vel.y = self->next_vel.y * drag_coef;
  self->next_vel = (Vector2){0.0f, 0.0f};
  self->vel_start_time = current_time();
}