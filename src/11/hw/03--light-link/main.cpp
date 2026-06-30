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

struct Block
{
  double x;
  double y;
  double z;
  double w;
  double h;
  double d;
  double angle;
  std::array<float, 4> color;
};

MouseState mouse_state;
ViewState view_state;
LightState light_state;

cv::VideoCapture capture;
int frame_count = 0;
double frame_rate = 30.0;

std::vector<Block> const blocks = {
  // A (4 blocks) - Red-orange
  { -280.0, 120.0, 0.0, 20.0, 100.0, 20.0, 0.0, {0.9f, 0.3f, 0.1f, 1.0f} },
  { -220.0, 120.0, 0.0, 20.0, 100.0, 20.0, 0.0, {0.9f, 0.3f, 0.1f, 1.0f} },
  { -250.0, 160.0, 0.0, 40.0, 20.0, 20.0, 0.0, {0.9f, 0.3f, 0.1f, 1.0f} },
  { -250.0, 120.0, 0.0, 40.0, 20.0, 20.0, 0.0, {0.9f, 0.3f, 0.1f, 1.0f} },

  // I (1 block) - Yellow-green
  { -130.0, 120.0, 0.0, 20.0, 100.0, 20.0, 0.0, {0.5f, 0.8f, 0.1f, 1.0f} },

  // T (2 blocks) - Blue
  { -20.0, 160.0, 0.0, 80.0, 20.0, 20.0, 0.0, {0.1f, 0.5f, 0.9f, 1.0f} },
  { -20.0, 110.0, 0.0, 20.0, 80.0, 20.0, 0.0, {0.1f, 0.5f, 0.9f, 1.0f} },

  // K (3 blocks) - Purple
  { -310.0, -120.0, 0.0, 20.0, 100.0, 20.0, 0.0, {0.7f, 0.2f, 0.8f, 1.0f} },
  { -270.0, -95.0, 0.0, 20.0, 60.0, 20.0, -45.0, {0.7f, 0.2f, 0.8f, 1.0f} },
  { -270.0, -145.0, 0.0, 20.0, 60.0, 20.0, 45.0, {0.7f, 0.2f, 0.8f, 1.0f} },

  // 2 (5 blocks) - Magenta
  { -180.0, -80.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.9f, 0.1f, 0.6f, 1.0f} },
  { -180.0, -120.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.9f, 0.1f, 0.6f, 1.0f} },
  { -180.0, -160.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.9f, 0.1f, 0.6f, 1.0f} },
  { -160.0, -100.0, 0.0, 20.0, 20.0, 20.0, 0.0, {0.9f, 0.1f, 0.6f, 1.0f} },
  { -200.0, -140.0, 0.0, 20.0, 20.0, 20.0, 0.0, {0.9f, 0.1f, 0.6f, 1.0f} },

  // 4 (3 blocks) - Cyan
  { -100.0, -100.0, 0.0, 20.0, 60.0, 20.0, 0.0, {0.1f, 0.8f, 0.8f, 1.0f} },
  { -80.0, -120.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.1f, 0.8f, 0.8f, 1.0f} },
  { -60.0, -120.0, 0.0, 20.0, 100.0, 20.0, 0.0, {0.1f, 0.8f, 0.8f, 1.0f} },

  // 1 (1 block) - Green
  { 10.0, -120.0, 0.0, 20.0, 100.0, 20.0, 0.0, {0.2f, 0.8f, 0.3f, 1.0f} },

  // 3 (5 blocks) - Orange
  { 100.0, -80.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.9f, 0.5f, 0.1f, 1.0f} },
  { 100.0, -120.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.9f, 0.5f, 0.1f, 1.0f} },
  { 100.0, -160.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.9f, 0.5f, 0.1f, 1.0f} },
  { 120.0, -100.0, 0.0, 20.0, 20.0, 20.0, 0.0, {0.9f, 0.5f, 0.1f, 1.0f} },
  { 120.0, -140.0, 0.0, 20.0, 20.0, 20.0, 0.0, {0.9f, 0.5f, 0.1f, 1.0f} },

  // 2 (5 blocks) - Light violet
  { 200.0, -80.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.6f, 0.5f, 0.9f, 1.0f} },
  { 200.0, -120.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.6f, 0.5f, 0.9f, 1.0f} },
  { 200.0, -160.0, 0.0, 60.0, 20.0, 20.0, 0.0, {0.6f, 0.5f, 0.9f, 1.0f} },
  { 220.0, -100.0, 0.0, 20.0, 20.0, 20.0, 0.0, {0.6f, 0.5f, 0.9f, 1.0f} },
  { 180.0, -140.0, 0.0, 20.0, 20.0, 20.0, 0.0, {0.6f, 0.5f, 0.9f, 1.0f} }
};

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

void draw_block(Block const &b)
{
  glPushMatrix();
  glTranslated(b.x, b.y, b.z);
  glRotated(b.angle, 0.0, 0.0, 1.0);
  glScaled(b.w, b.h, b.d);

  GLfloat const mat_diffuse[] = {b.color[0], b.color[1], b.color[2], b.color[3]};
  GLfloat const mat_ambient[] = {b.color[0] * 0.3f, b.color[1] * 0.3f, b.color[2] * 0.3f, b.color[3]};
  GLfloat const mat_specular[] = {0.3f, 0.3f, 0.3f, 1.0f};
  GLfloat const mat_shininess = 30.0f;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

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

  if (light_state.detected)
  {
    glEnable(GL_LIGHT0);
    GLfloat const pos0[] = {static_cast<float>(light_state.x), static_cast<float>(light_state.y), 200.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
  }
  else
  {
    glDisable(GL_LIGHT0);
  }

  draw_backboard();

  for (auto const &b : blocks)
  {
    draw_block(b);
  }

  if (light_state.detected)
  {
    glPushMatrix();
    glTranslated(light_state.x, light_state.y, 200.0);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.9f);
    glutSolidSphere(12.0, 16, 16);
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
  glutCreateWindow("Linked Light Source");
  glutInitWindowPosition(0, 0);

  glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);

  GLfloat const diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat const specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
  GLfloat const ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};

  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

  GLfloat const global_ambient[] = {0.05f, 0.05f, 0.07f, 1.0f};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

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
