#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <ranges>
#include <vector>

#include <GLUT/glut.h>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

constexpr auto COLS = 32;
constexpr auto ROWS = 24;

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
};

struct MirrorState
{
  std::array<std::array<double, COLS>, ROWS> current_angles{};
  std::array<std::array<double, COLS>, ROWS> target_angles{};
};

MouseState mouse_state;
ViewState view_state;
MirrorState mirror_state;

cv::VideoCapture capture;
int frame_count = 0;
double frame_rate = 30.0;

void draw_backboard()
{
  glPushMatrix();
  glTranslated(0.0, 0.0, -5.0);
  glScaled(600.0, 460.0, 2.0);

  GLfloat const mat_diffuse[] = {0.15f, 0.15f, 0.15f, 1.0f};
  GLfloat const mat_ambient[] = {0.05f, 0.05f, 0.05f, 1.0f};
  GLfloat const mat_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};
  GLfloat const mat_shininess = 0.0f;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

  glutSolidCube(1.0);
  glPopMatrix();
}

void draw_plate(int r, int c, double angle)
{
  auto const plate_w = 16.0;
  auto const plate_h = 16.0;
  auto const plate_d = 4.0;
  auto const spacing = 2.0;

  auto const x = (c - (COLS - 1) / 2.0) * (plate_w + spacing);
  auto const y = (((ROWS - 1) / 2.0) - r) * (plate_h + spacing);

  glPushMatrix();
  glTranslated(x, y, 0.0);
  glRotated(angle, 1.0, 0.0, 0.0);
  glScaled(plate_w, plate_h, plate_d);

  GLfloat const mat_diffuse[] = {0.6f, 0.45f, 0.3f, 1.0f};
  GLfloat const mat_ambient[] = {0.25f, 0.18f, 0.12f, 1.0f};
  GLfloat const mat_specular[] = {0.1f, 0.1f, 0.1f, 1.0f};
  GLfloat const mat_shininess = 10.0f;

  glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

  glutSolidCube(1.0);
  glPopMatrix();
}

cv::Mat crop_to_aspect_ratio(const cv::Mat &src, double target_aspect)
{
  auto const src_w = src.cols;
  auto const src_h = src.rows;
  auto const src_aspect = static_cast<double>(src_w) / src_h;

  if (std::abs(src_aspect - target_aspect) < 0.001)
  {
    return src;
  }

  if (src_aspect > target_aspect)
  {
    auto const new_w = static_cast<int>(src_h * target_aspect);
    auto const offset_x = (src_w - new_w) / 2;
    return src(cv::Rect(offset_x, 0, new_w, src_h));
  }
  else
  {
    auto const new_h = static_cast<int>(src_w / target_aspect);
    auto const offset_y = (src_h - new_h) / 2;
    return src(cv::Rect(0, offset_y, src_w, new_h));
  }
}

void update_angles()
{
  auto original = cv::Mat();
  auto gray = cv::Mat();
  auto color_img = cv::Mat();

  capture >> original;
  if (original.empty())
  {
    return;
  }
  auto const cropped = crop_to_aspect_ratio(original, static_cast<double>(COLS) / ROWS);
  cv::flip(cropped, color_img, 1);
  auto lowres = cv::Mat();
  cv::resize(color_img, lowres, cv::Size(COLS, ROWS));
  cv::cvtColor(lowres, gray, cv::COLOR_BGR2GRAY);

  for (auto r = 0; r < ROWS; ++r)
  {
    for (auto c = 0; c < COLS; ++c)
    {
      auto const val = gray.at<uchar>(r, c);
      auto const norm = val / 255.0;
      mirror_state.target_angles[r][c] = (norm - 0.5) * 90.0;
    }
  }

  for (auto r = 0; r < ROWS; ++r)
  {
    for (auto c = 0; c < COLS; ++c)
    {
      mirror_state.current_angles[r][c] +=
          (mirror_state.target_angles[r][c] - mirror_state.current_angles[r][c]) * 0.15;
    }
  }

  auto display_mat = cv::Mat();
  cv::resize(color_img, display_mat, cv::Size(640, 480), 0, 0, cv::INTER_LINEAR);
  mat_util::show("Frame", display_mat, 480);
}

void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  auto const ex =
      view_state.dist * std::cos(view_state.deg_x * M_PI / 180.0) * std::sin(view_state.deg_y * M_PI / 180.0);
  auto const ey = view_state.dist * std::sin(view_state.deg_x * M_PI / 180.0);
  auto const ez =
      view_state.dist * std::cos(view_state.deg_x * M_PI / 180.0) * std::cos(view_state.deg_y * M_PI / 180.0);

  gluLookAt(ex, ey, ez, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  GLfloat const pos0[] = {200.0f, 700.0f, 200.0f, 0.0f};
  glLightfv(GL_LIGHT0, GL_POSITION, pos0);

  draw_backboard();

  for (auto r = 0; r < ROWS; ++r)
  {
    for (auto c = 0; c < COLS; ++c)
    {
      draw_plate(r, c, mirror_state.current_angles[r][c]);
    }
  }

  glutSwapBuffers();
}

void timer(int value)
{
  update_angles();
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
}

void init_gl()
{
  glutInitWindowSize(640, 480);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Wooden Mirror Simulation");
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
  init_gl();
  init_capture();
  glutMainLoop();
  return 0;
}
