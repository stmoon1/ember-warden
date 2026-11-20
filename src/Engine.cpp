#ifdef _WIN32
#define NOMINMAX
// clang-format off
#include <windows.h>
#include <imm.h>
// clang-format on
#endif
#include "Engine.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <glad/gl.h>
#include <iterator>
#include <stdexcept>
#define GLFW_INCLUDE_NONE
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#include <stb_image.h>
namespace citadel::graphics {
std::filesystem::path asset(const std::string &name) {
  std::filesystem::path executable = std::filesystem::current_path();
#ifdef _WIN32
  std::wstring location(32768, L'\0');
  auto size =
      GetModuleFileNameW(nullptr, location.data(), DWORD(location.size()));
  if (size > 0 && size < location.size()) {
    location.resize(size);
    executable = std::filesystem::path(location).parent_path();
  }
#endif
  for (const auto &directory :
       {executable / "assets", std::filesystem::current_path() / "assets",
        std::filesystem::path(CITADEL_ASSET_DIR)}) {
    auto candidate = directory / name;
    if (std::filesystem::is_regular_file(candidate))
      return candidate;
  }
  throw std::runtime_error("Missing game asset: " + name);
}
unsigned compileProgram(const std::string &vertex,
                        const std::string &fragment) {
  auto compile = [](unsigned kind, const std::string &text) {
    unsigned shader = glCreateShader(kind);
    const char *code = text.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);
    int ready = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ready);
    if (!ready) {
      std::array<char, 4096> error{};
      glGetShaderInfoLog(shader, int(error.size()), nullptr, error.data());
      glDeleteShader(shader);
      throw std::runtime_error(error.data());
    }
    return shader;
  };
  unsigned vs = compile(GL_VERTEX_SHADER, vertex), fs = 0;
  try {
    fs = compile(GL_FRAGMENT_SHADER, fragment);
  } catch (...) {
    glDeleteShader(vs);
    throw;
  }
  unsigned program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);
  int ready = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ready);
  if (!ready) {
    std::array<char, 4096> error{};
    glGetProgramInfoLog(program, int(error.size()), nullptr, error.data());
    glDeleteProgram(program);
    throw std::runtime_error(error.data());
  }
  return program;
}
unsigned loadTexture(const std::filesystem::path &path) {
  int width, height, channels;
  auto *pixels =
      stbi_load(path.u8string().c_str(), &width, &height, &channels, 4);
  if (!pixels)
    throw std::runtime_error("Cannot decode texture: " + path.u8string());
  unsigned texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return texture;
}
Window::Window(int width, int height, const std::string &title,
               bool fullscreen) {
  if (!glfwInit())
    throw std::runtime_error("GLFW initialization failed");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  if (std::getenv("CITADEL_SMOKE_TEST"))
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  windowWidth = width;
  windowHeight = height;
  auto *monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
  const auto *mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
  if (mode) {
    width = mode->width;
    height = mode->height;
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  }
  handle = glfwCreateWindow(width, height, title.c_str(),
                            mode ? monitor : nullptr, nullptr);
  if (!handle) {
    glfwTerminate();
    throw std::runtime_error("An OpenGL 3.3 capable display is required");
  }
  // GLFW owns a copy after this call; missing optional artwork is non-fatal.
  try {
    int iconWidth = 0, iconHeight = 0, iconChannels = 0;
    auto *iconPixels =
        stbi_load(asset("icons/ember-warden.png").u8string().c_str(), &iconWidth,
                  &iconHeight, &iconChannels, 4);
    if (iconPixels) {
      GLFWimage icon{iconWidth, iconHeight, iconPixels};
      glfwSetWindowIcon(handle, 1, &icon);
      stbi_image_free(iconPixels);
    }
  } catch (const std::exception &) {
    // The embedded executable icon remains available on Windows.
  }
  glfwMakeContextCurrent(handle);
  if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
    glfwDestroyWindow(handle);
    glfwTerminate();
    throw std::runtime_error("OpenGL loader failed");
  }
  glfwSwapInterval(1);
}
Window::~Window() {
  if (handle)
    glfwDestroyWindow(handle);
  glfwTerminate();
}
bool Window::shouldClose() const { return glfwWindowShouldClose(handle); }
bool Window::keyDown(int key) const {
  if (!glfwGetWindowAttrib(handle, GLFW_FOCUSED))
    return false;
#ifdef _WIN32
  // IMEs can replace key messages with VK_PROCESSKEY. Poll the physical
  // key state only while this game's window is in the foreground.
  if (GetForegroundWindow() != glfwGetWin32Window(handle))
    return false;
  int vk = 0;
  if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z) ||
      (key >= GLFW_KEY_0 && key <= GLFW_KEY_9))
    vk = key;
  else if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)
    vk = VK_F1 + key - GLFW_KEY_F1;
  else
    switch (key) {
    case GLFW_KEY_LEFT_SHIFT:
      vk = VK_LSHIFT;
      break;
    case GLFW_KEY_RIGHT_SHIFT:
      vk = VK_RSHIFT;
      break;
    case GLFW_KEY_SPACE:
      vk = VK_SPACE;
      break;
    case GLFW_KEY_ESCAPE:
      vk = VK_ESCAPE;
      break;
    case GLFW_KEY_TAB:
      vk = VK_TAB;
      break;
    case GLFW_KEY_MINUS:
      vk = VK_OEM_MINUS;
      break;
    case GLFW_KEY_EQUAL:
      vk = VK_OEM_PLUS;
      break;
    default:
      break;
    }
  if (vk)
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
#endif
  return glfwGetKey(handle, key) == GLFW_PRESS;
}
void Window::pollEvents() { glfwPollEvents(); }
void Window::requestClose() { glfwSetWindowShouldClose(handle, GL_TRUE); }
bool Window::fullscreen() const {
  return glfwGetWindowMonitor(handle) != nullptr;
}
void Window::setFullscreen(bool enabled) {
  if (enabled == fullscreen())
    return;
  if (enabled) {
    auto *monitor = glfwGetPrimaryMonitor();
    const auto *mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (!mode)
      return;
    glfwGetWindowPos(handle, &windowX, &windowY);
    glfwGetWindowSize(handle, &windowWidth, &windowHeight);
    glfwSetWindowMonitor(handle, monitor, 0, 0, mode->width, mode->height,
                         mode->refreshRate);
  } else {
    glfwSetWindowMonitor(handle, nullptr, windowX, windowY, windowWidth,
                         windowHeight, 0);
  }
}
void Window::swapBuffers() { glfwSwapBuffers(handle); }
std::pair<int, int> Window::framebufferSize() const {
  int w, h;
  glfwGetFramebufferSize(handle, &w, &h);
  return {w, h};
}
FirstPersonCamera::FirstPersonCamera(glm::vec3 p, float y, float tilt)
    : eye(p), yaw(y), pitch(tilt) {
  orient();
}
void FirstPersonCamera::orient() {
  float y = glm::radians(yaw), p = glm::radians(pitch);
  direction =
      glm::normalize(glm::vec3(cos(y) * cos(p), sin(p), sin(y) * cos(p)));
}
void FirstPersonCamera::rotate(float horizontal, float vertical) {
  yaw += horizontal;
  pitch = std::clamp(pitch + vertical, -85.f, 85.f);
  orient();
}
void FirstPersonCamera::movePlanar(float amount, float strafe) {
  glm::vec3 front = glm::normalize(glm::vec3(direction.x, 0, direction.z));
  eye += front * amount + glm::cross(front, glm::vec3(0, 1, 0)) * strafe;
}
glm::mat4 FirstPersonCamera::viewProjection(float aspect) const {
  return glm::perspective(glm::radians(65.f), aspect, .08f, 180.f) *
         glm::lookAt(eye, eye + direction, glm::vec3(0, 1, 0));
}
} // namespace citadel::graphics
namespace citadel::app {
AppUi::AppUi(graphics::Window &window) {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  // One owner controls cursor mode. UI draws its own pointer in fullscreen.
  io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
  io.MouseDrawCursor = true;
  ImGui::GetStyle().MouseCursorScale = 1.4f;
  glfwSetInputMode(window.nativeHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  ImFontConfig font;
  font.SizePixels = 18;
  io.FontDefault = io.Fonts->AddFontFromFileTTF(
      graphics::asset("fonts/NotoSansCJKsc-Regular.otf").u8string().c_str(), 18,
      &font);
  if (!io.FontDefault)
    throw std::runtime_error("CJK font could not be loaded");
  if (!ImGui_ImplGlfw_InitForOpenGL(window.nativeHandle(), true) ||
      !ImGui_ImplOpenGL3_Init("#version 330 core"))
    throw std::runtime_error("UI initialization failed");
}
AppUi::~AppUi() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
void AppUi::beginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}
void AppUi::render() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
FirstPersonController::~FirstPersonController() {
#ifdef _WIN32
  if (imeDetached && IsWindow(static_cast<HWND>(imeWindow)))
    ImmAssociateContext(static_cast<HWND>(imeWindow),
                        static_cast<HIMC>(imeContext));
#endif
}
void FirstPersonController::setMouseCapture(graphics::Window &window,
                                            bool enabled) {
  auto *handle = window.nativeHandle();
  bool focused = glfwGetWindowAttrib(handle, GLFW_FOCUSED) != 0;
  enabled = enabled && focused;
#ifdef _WIN32
  // Disable composition only for FPS control in this window. Restore the exact
  // previous context for menus/search; never switch the user's keyboard layout.
  if (enabled && !imeDetached) {
    auto native = glfwGetWin32Window(handle);
    imeWindow = native;
    imeContext = ImmAssociateContext(native, nullptr);
    imeDetached = true;
  } else if (!enabled && imeDetached) {
    ImmAssociateContext(static_cast<HWND>(imeWindow),
                        static_cast<HIMC>(imeContext));
    imeDetached = false;
    imeWindow = imeContext = nullptr;
  }
#endif
  bool released = captured && !enabled;
  if (captured != enabled) {
    captured = enabled;
    sampled = false;
  }
  // HIDDEN is free movement with a software pointer, DISABLED is FPS capture.
  int desired = captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_HIDDEN;
  if (glfwGetInputMode(handle, GLFW_CURSOR) != desired)
    glfwSetInputMode(handle, GLFW_CURSOR, desired);
  ImGui::GetIO().MouseDrawCursor = !captured && focused;
  if (released && focused) {
    int width = 0, height = 0;
    glfwGetWindowSize(handle, &width, &height);
    glfwSetCursorPos(handle, width * .5, height * .5);
    ImGui::GetIO().AddMousePosEvent(width * .5f, height * .5f);
  }
}
void FirstPersonController::update(graphics::Window &window,
                                   graphics::FirstPersonCamera &camera,
                                   float dt, bool keyboard, bool mouse) {
  auto *handle = window.nativeHandle();
  bool focused = glfwGetWindowAttrib(handle, GLFW_FOCUSED);
  mouse = mouse && focused;
  keyboard = keyboard && focused;
  setMouseCapture(window, mouse);
  if (mouse) {
    double x, y;
    glfwGetCursorPos(handle, &x, &y);
    if (sampled)
      camera.rotate(float(x - previousX) * .12f * mouseSensitivityScale,
                    float(previousY - y) * .12f * mouseSensitivityScale);
    previousX = x;
    previousY = y;
    sampled = true;
  }
  if (!keyboard)
    return;
  float forward =
      float(window.keyDown(GLFW_KEY_W)) - float(window.keyDown(GLFW_KEY_S));
  float side =
      float(window.keyDown(GLFW_KEY_D)) - float(window.keyDown(GLFW_KEY_A));
  float magnitude = std::max(1.f, std::sqrt(forward * forward + side * side));
  float speed = 4.5f * moveScale *
                ((window.keyDown(GLFW_KEY_LEFT_SHIFT) ||
                  window.keyDown(GLFW_KEY_RIGHT_SHIFT))
                     ? 1.55f
                     : 1.f);
  camera.movePlanar(forward / magnitude * speed * dt,
                    side / magnitude * speed * dt);
}
} // namespace citadel::app
