#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <utility>
struct GLFWwindow;
namespace citadel::graphics {
std::filesystem::path asset(const std::string &name);
unsigned compileProgram(const std::string &vertex, const std::string &fragment);
unsigned loadTexture(const std::filesystem::path &path);
class Window {
  GLFWwindow *handle = nullptr;
  int windowX = 80, windowY = 60, windowWidth = 1440, windowHeight = 900;

public:
  Window(int width, int height, const std::string &title,
         bool fullscreen = true);
  ~Window();
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  GLFWwindow *nativeHandle() const { return handle; }
  bool shouldClose() const;
  bool keyDown(int key) const;
  void pollEvents();
  void requestClose();
  bool fullscreen() const;
  void setFullscreen(bool enabled);
  void swapBuffers();
  std::pair<int, int> framebufferSize() const;
};
class FirstPersonCamera {
  glm::vec3 eye, direction;
  float yaw, pitch;
  void orient();

public:
  FirstPersonCamera(glm::vec3 p = {8, 4.65f, 12}, float y = -65,
                    float tilt = -18);
  const glm::vec3 &position() const { return eye; }
  const glm::vec3 &forward() const { return direction; }
  void setPosition(glm::vec3 p) { eye = p; }
  void rotate(float horizontal, float vertical);
  void movePlanar(float forward, float strafe);
  glm::mat4 viewProjection(float aspect) const;
};
} // namespace citadel::graphics
namespace citadel::app {
class AppUi {
public:
  explicit AppUi(graphics::Window &window);
  ~AppUi();
  void beginFrame();
  void render();
};
class FirstPersonController {
  bool captured = false, sampled = false;
  double previousX = 0, previousY = 0;
#ifdef _WIN32
  void *imeWindow = nullptr, *imeContext = nullptr;
  bool imeDetached = false;
#endif

public:
  FirstPersonController() = default;
  ~FirstPersonController();
  FirstPersonController(const FirstPersonController &) = delete;
  FirstPersonController &operator=(const FirstPersonController &) = delete;
  float mouseSensitivityScale = .5f, moveScale = 1;
  void setMouseCapture(graphics::Window &window, bool enabled);
  void update(graphics::Window &window, graphics::FirstPersonCamera &camera,
              float dt, bool keyboard, bool mouse);
};
} // namespace citadel::app
