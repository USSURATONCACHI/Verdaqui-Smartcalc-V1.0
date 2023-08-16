#ifndef SRC_CODE_UTIL_CAMERA_H_
#define SRC_CODE_UTIL_CAMERA_H_

typedef struct Vector2 {
  float x;
  float y;
} Vector2;

#define CAMERA_ZOOM_EXP 1000.0
#define CAMERA_VEL_EXP 128.0

typedef struct PlotCamera {
  Vector2 pos;
  Vector2 vel;
  double vel_start_time;

  Vector2 next_vel;
  double last_drag_time;
  double vel_exp;
  float vel_inertia;

  float zoom;
  float zoom_vel;
  double zoom_vel_start_time;
  double zoom_exp;
} PlotCamera;

PlotCamera PlotCamera_new(float start_x, float start_y);
Vector2 PlotCamera_pos(const PlotCamera* self);
Vector2 PlotCamera_vel(const PlotCamera* self);
void PlotCamera_set_pos(PlotCamera* self, Vector2 new_pos);
void PlotCamera_wrap_x(PlotCamera* self, float max);
void PlotCamera_wrap_y(PlotCamera* self, float max);
void PlotCamera_update_anim(PlotCamera* self);
float PlotCamera_zoom(const PlotCamera* self);
float PlotCamera_zoom_vel(const PlotCamera* self);
void PlotCamera_set_zoom(PlotCamera* self, float new_zoom);
void PlotCamera_on_zoom(PlotCamera* self, float delta);
void PlotCamera_on_drag(PlotCamera* self, Vector2 drag);
void PlotCamera_on_drag_start(PlotCamera* self);
void PlotCamera_on_drag_end(PlotCamera* self);

#endif  // SRC_CODE_UTIL_CAMERA_H_