#include <cstdlib>
#include <vector>
#include <fstream>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>
#include <GL/glfw.h>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

static GLuint load_shader(GLenum type, const char *filename);
static GLuint build_program();

static GLuint create_array_buffer(GLenum type, GLsizeiptr size, const GLvoid *data);

static void iterate();

static GLFWCALL void on_key(int key, int action);
static EM_BOOL on_em_mousemove(int event_type, const EmscriptenMouseEvent *mouse_event, void *user_data);

static GLuint mvp_id;

static bool keys[GLFW_KEY_LAST];

static float pos_x = 0, pos_y = -4;
static float delta_z = 0, delta_x = 0;

struct Model
{
  std::vector<GLfloat> positions;
  GLuint positions_id;

  std::vector<GLfloat> colors;
  GLuint colors_id;

  std::vector<GLushort> elements;
  GLuint id;
};

static void load_obj(const char *filename, Model *model);

static Model suzanne;
static Model teapot;
static Model bunny;

int main()
{
  if (!glfwInit())
    exit(EXIT_FAILURE);

  if (!glfwOpenWindow(640, 480, 8, 8, 8, 8, 16, 0, GLFW_WINDOW))
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  if (glewInit() != GLEW_OK)
  {
    glfwCloseWindow();
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(on_key);
  emscripten_set_mousemove_callback(nullptr, nullptr, false, on_em_mousemove);

  const GLuint program = build_program();
  glUseProgram(program);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  mvp_id = glGetUniformLocation(program, "mvp");

  load_obj("/data/models/suzanne.obj", &suzanne);
  load_obj("/data/models/teapot.obj", &teapot);
  load_obj("/data/models/bunny.obj", &bunny);

  glViewport(0, 0, 640, 480);

  glEnable(GL_DEPTH_TEST);

  glClearColor(0, 0, 0, 0);

  emscripten_set_main_loop(iterate, 0, 1);
}

GLuint load_shader(const GLenum type, const char *filename)
{
  FILE *file = fopen(filename, "r");

  fseek(file, 0, SEEK_END);
  const long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *source = (char*)malloc(size + 1);
  fread(source, size, 1, file);
  source[size] = 0;

  fclose(file);

  const GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, (const GLchar**)&source, nullptr);
  glCompileShader(shader);

  free(source);

  return shader;
}

GLuint build_program()
{
  const GLuint vertex_shader = load_shader(GL_VERTEX_SHADER, "/data/shaders/vertex.glsl");
  const GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER, "/data/shaders/fragment.glsl");

  const GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glBindAttribLocation(program, 0, "position");
  glBindAttribLocation(program, 1, "color");
  glLinkProgram(program);
  return program;
}

GLuint create_array_buffer(GLenum type, GLsizeiptr size, const GLvoid *data)
{
  GLuint id;
  glGenBuffers(1, &id);
  glBindBuffer(type, id);
  glBufferData(type, size, data, GL_STATIC_DRAW);
  return id;
}

void iterate()
{
  if (keys['W'])
  {
    pos_x += 0.1 * sin(glm::radians(delta_z));
    pos_y += 0.1 * cos(glm::radians(delta_z));
  }

  if (keys['S'])
  {
    pos_x -= 0.1 * sin(glm::radians(delta_z));
    pos_y -= 0.1 * cos(glm::radians(delta_z));
  }

  if (keys['A'])
  {
    pos_x -= 0.1 * cos(glm::radians(delta_z));
    pos_y += 0.1 * sin(glm::radians(delta_z));
  }

  if (keys['D'])
  {
    pos_x += 0.1 * cos(glm::radians(delta_z));
    pos_y -= 0.1 * sin(glm::radians(delta_z));
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-pos_x, -pos_y, -2.0f));

  glm::mat4 view = glm::mat4(1.0f)
    * glm::rotate(glm::mat4(1.0f), glm::radians(delta_x - 90), glm::vec3(1.0f, 0.0f, 0.0f))
    * glm::rotate(glm::mat4(1.0f), glm::radians(delta_z), glm::vec3(0.0f, 0.0f, 1.0f));

  glm::mat4 projection = glm::perspective(45.0f, (float)640 / (float)480, 0.1f, 10.0f);

  glm::mat4 mvp = projection * view * model;

  glUniformMatrix4fv(mvp_id, 1, GL_FALSE, glm::value_ptr(mvp));

  auto draw_model = [](Model &model) {
    glBindBuffer(GL_ARRAY_BUFFER, model.positions_id);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid*>(0));

    glBindBuffer(GL_ARRAY_BUFFER, model.colors_id);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid*>(0));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.id);

    glDrawElements(GL_TRIANGLES, model.elements.size(), GL_UNSIGNED_SHORT, 0);
  };

  draw_model(suzanne);

  mvp = glm::translate(mvp, glm::vec3(4.0f, 0.0f, 0.0f))
    * glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  glUniformMatrix4fv(mvp_id, 1, GL_FALSE, glm::value_ptr(mvp));

  draw_model(teapot);

  mvp = glm::translate(mvp, glm::vec3(-4.0f, 0.0f, 0.0f))
    * glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  glUniformMatrix4fv(mvp_id, 1, GL_FALSE, glm::value_ptr(mvp));

  draw_model(bunny);
}

GLFWCALL void on_key(int key, int action)
{
  keys[key] = action != GLFW_RELEASE;
}

EM_BOOL on_em_mousemove(int event_type, const EmscriptenMouseEvent *mouse_event, void *user_data)
{
  delta_z += mouse_event->movementX;
  delta_x += mouse_event->movementY;

  if (delta_z < 0)
    delta_z = 359;
  else
  if (delta_z >= 360)
    delta_z = 0;

  if (delta_x < -90)
    delta_x = -90;
  else
  if (delta_x > 90)
    delta_x = 90;

  return true;
}

void load_obj(const char *filename, Model *model)
{
  std::ifstream file(filename, std::ios::in);

  std::string line;
  while (std::getline(file, line))
  {
    if (line.substr(0,2) == "v ")
    {
      std::istringstream s(line.substr(2));
      GLfloat x, y, z;
      s >> x; s >> y, s >> z;
      model->positions.push_back(x);
      model->positions.push_back(y);
      model->positions.push_back(z);
      model->colors.push_back(1.0f);
      model->colors.push_back(0.0f);
      model->colors.push_back(0.0f);
    }
    else
    if (line.substr(0,2) == "f ")
    {
      std::istringstream s(line.substr(2));
      GLushort a, b, c;
      s >> a;
      s >> b;
      s >> c;
      model->elements.push_back(a - 1);
      model->elements.push_back(b - 1);
      model->elements.push_back(c - 1);
    }
  }

  model->positions_id = create_array_buffer(GL_ARRAY_BUFFER, model->positions.size() * sizeof(GLfloat), model->positions.data());
  model->colors_id = create_array_buffer(GL_ARRAY_BUFFER, model->positions.size() * sizeof(GLfloat), model->colors.data());
  model->id = create_array_buffer(GL_ELEMENT_ARRAY_BUFFER, model->elements.size() * sizeof(GLushort), model->elements.data());
}
