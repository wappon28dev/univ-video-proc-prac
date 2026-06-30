#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <ranges>
#include <vector>

#include <GLUT/glut.h>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

struct MouseState
{
  int x = 0;
  int y = 0;
  int state = 0;
  int button = 0;
};

struct ViewState
{
  double dist = 1200.0;
  double deg_x = 0.0;
  double deg_y = 0.0;
  double aspect = 4.0 / 3.0;
};

struct LightState
{
  bool detected = false;
  double x = 0.0;
  double y = 0.0;
};

struct MothState
{
  double x = 0.0;
  double y = 0.0;
  double angle = 0.0;
};

MouseState mouse_state;
ViewState view_state;
LightState light_state;
MothState moth_state;

cv::VideoCapture capture;
int frame_count = 0;
double frame_rate = 30.0;

void draw_backboard()
{
  glPushMatrix();
  glTranslated(0.0, 0.0, -15.0);

  auto const visible_h = 2.0 * view_state.dist * std::tan(15.0 * M_PI / 180.0);
  auto const visible_w = visible_h * view_state.aspect;
  glScaled(visible_w * 1.5, visible_h * 1.5, 2.0);

  GLfloat const mat_diffuse[] = {0.1f, 0.1f, 0.12f, 1.0f};
  GLfloat const mat_ambient[] = {0.05f, 0.05f, 0.06f, 1.0f};
  GLfloat const mat_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};
  GLfloat const mat_shininess = 0.0f;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

  glutSolidCube(1.0);
  glPopMatrix();
}

void draw_moth()
{
  GLfloat const mat_diffuse[] = {0.1f, 0.8f, 0.2f, 1.0f};
  GLfloat const mat_ambient[] = {0.05f, 0.3f, 0.1f, 1.0f};
  GLfloat const mat_specular[] = {0.6f, 0.6f, 0.6f, 1.0f};
  GLfloat const mat_shininess = 80.0f;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

  glPushMatrix();
  glTranslated(moth_state.x, moth_state.y, 0.0);
  glRotated(moth_state.angle, 0.0, 0.0, 1.0);
  glScaled(60.0, 24.0, 16.0);
  glutSolidCube(1.0);
  glPopMatrix();

  GLfloat const wing_diffuse[] = {0.9f, 0.9f, 0.9f, 0.8f};
  GLfloat const wing_ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};
  GLfloat const wing_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat const wing_shininess = 100.0f;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, wing_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, wing_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR, wing_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, wing_shininess);

  auto const wing_flap = 35.0 * std::sin(frame_count * 1.5);

  glPushMatrix();
  glTranslated(moth_state.x, moth_state.y, 8.0);
  glRotated(moth_state.angle, 0.0, 0.0, 1.0);
  glTranslated(0.0, 12.0, 0.0);
  glRotated(wing_flap, 1.0, 0.0, 0.0);
  glScaled(24.0, 36.0, 2.0);
  glutSolidCube(1.0);
  glPopMatrix();

  glPushMatrix();
  glTranslated(moth_state.x, moth_state.y, 8.0);
  glRotated(moth_state.angle, 0.0, 0.0, 1.0);
  glTranslated(0.0, -12.0, 0.0);
  glRotated(-wing_flap, 1.0, 0.0, 0.0);
  glScaled(24.0, 36.0, 2.0);
  glutSolidCube(1.0);
  glPopMatrix();
}

void update_simulation()
{
  auto original = cv::Mat();
  auto gray = cv::Mat();
  auto color_img = cv::Mat();

  auto const visible_h = 2.0 * view_state.dist * std::tan(15.0 * M_PI / 180.0);
  auto const visible_w = visible_h * view_state.aspect;

  capture >> original;
  if (original.empty())
  {
    return;
  }
  auto const cam_w = original.cols;
  auto const cam_h = original.rows;

  cv::flip(original, color_img, 1);
  cv::cvtColor(color_img, gray, cv::COLOR_BGR2GRAY);

  double min_val = 0.0;
  double max_val = 0.0;
  cv::Point min_loc;
  cv::Point max_loc;
  cv::minMaxLoc(gray, &min_val, &max_val, &min_loc, &max_loc);

  if (max_val > 220.0)
  {
    light_state.detected = true;
    light_state.x = ((max_loc.x / static_cast<double>(cam_w)) - 0.5) * visible_w;
    light_state.y = (0.5 - (max_loc.y / static_cast<double>(cam_h))) * visible_h;

    cv::circle(color_img, max_loc, 15, cv::Scalar(0, 0, 255), 3);
  }
  else
  {
    light_state.detected = false;
  }

  if (light_state.detected)
  {
    auto const dx = light_state.x - moth_state.x;
    auto const dy = light_state.y - moth_state.y;
    auto const dist = std::hypot(dx, dy);

    if (dist > 5.0)
    {
      moth_state.angle = std::atan2(dy, dx) * 180.0 / M_PI;
      moth_state.x += dx * 0.08;
      moth_state.y += dy * 0.08;
    }
  }
  else
  {
    auto const time_sec = frame_count / frame_rate;
    auto const target_wander_x = (visible_w / 4.0) * std::cos(time_sec * 0.5) + (visible_w / 12.0) * std::sin(time_sec * 1.2);
    auto const target_wander_y = (visible_h / 4.0) * std::sin(time_sec * 0.6) + (visible_h / 12.0) * std::cos(time_sec * 1.5);

    auto const dx = target_wander_x - moth_state.x;
    auto const dy = target_wander_y - moth_state.y;
    auto const dist = std::hypot(dx, dy);

    if (dist > 5.0)
    {
      moth_state.angle = std::atan2(dy, dx) * 180.0 / M_PI;
      moth_state.x += dx * 0.03;
      moth_state.y += dy * 0.03;
    }
  }

  auto display_mat = cv::Mat();
  auto const display_w = static_cast<int>(480.0 * view_state.aspect);
  cv::resize(color_img, display_mat, cv::Size(display_w, 480), 0, 0, cv::INTER_NEAREST);
  mat_util::show("Frame", display_mat, 480);
}

void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  auto const ex = view_state.dist * std::cos(view_state.deg_x * M_PI / 180.0) * std::sin(view_state.deg_y * M_PI / 180.0);
  auto const ey = view_state.dist * std::sin(view_state.deg_x * M_PI / 180.0);
  auto const ez = view_state.dist * std::cos(view_state.deg_x * M_PI / 180.0) * std::cos(view_state.deg_y * M_PI / 180.0);

  gluLookAt(ex, ey, ez, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  GLfloat const pos0[] = {200.0f, 700.0f, 200.0f, 0.0f};
  glLightfv(GL_LIGHT0, GL_POSITION, pos0);

  draw_backboard();
  draw_moth();

  if (light_state.detected)
  {
    glPushMatrix();
    glTranslated(light_state.x, light_state.y, 0.0);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.2f);
    glutSolidSphere(16.0, 20, 20);
    glEnable(GL_LIGHTING);
    glPopMatrix();
  }

  glutSwapBuffers();
}

void timer(int value)
{
  update_simulation();
  glutPostRedisplay();
  cv::waitKey(1);
  frame_count++;
  glutTimerFunc(static_cast<unsigned int>(1000 / frame_rate), timer, 0);
}

void reshape(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30.0, static_cast<double>(w) / static_cast<double>(h), 1.0, 10000.0);
  glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state_val, int x, int y)
{
  if (state_val == GLUT_DOWN)
  {
    mouse_state.x = x;
    mouse_state.y = y;
    mouse_state.state = state_val;
    mouse_state.button = button;
  }
}

void motion(int x, int y)
{
  if (mouse_state.button == GLUT_RIGHT_BUTTON)
  {
    view_state.deg_y += (mouse_state.x - x) * 0.5;
    view_state.deg_x += (y - mouse_state.y) * 0.5;
  }
  mouse_state.x = x;
  mouse_state.y = y;
}

void keyboard(unsigned char key, int x, int y)
{
  switch (key)
  {
  case 'q':
  case 'Q':
  case 27:
    if (capture.isOpened())
    {
      capture.release();
    }
    std::exit(0);
  case 'r':
  case 'R':
    view_state.dist = 1200.0;
    view_state.deg_x = 0.0;
    view_state.deg_y = 0.0;
    break;
  }
}

void init_capture()
{
  capture.open(0);
  if (!capture.isOpened())
  {
    std::cerr << "Error: Camera not found." << std::endl;
    std::exit(-1);
  }

  auto const w = capture.get(cv::CAP_PROP_FRAME_WIDTH);
  auto const h = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
  if (w > 0 && h > 0)
  {
    view_state.aspect = static_cast<double>(w) / h;
  }
}

void init_gl()
{
  auto const win_w = static_cast<int>(480.0 * view_state.aspect);
  glutInitWindowSize(win_w, 480);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Phototaxis Moth Simulation");
  glutInitWindowPosition(0, 0);

  glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc(keyboard);
  glutTimerFunc(static_cast<unsigned int>(1000 / frame_rate), timer, 0);
}

int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  init_capture();
  init_gl();
  glutMainLoop();
  return 0;
}
