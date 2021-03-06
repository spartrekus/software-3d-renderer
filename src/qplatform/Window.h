#include <qsoft/qsoft.h>

#include <sr1/zero_initialized>
#include <memory>

class Platform;
class Camera;

class Window
{
  friend class Platform;
  friend class Camera;

public:
  int getWidth();
  int getHeight();
  std::shared_ptr<qsoft::Renderer> getRenderer();
  std::shared_ptr<Camera> getCurrentCamera();
  std::shared_ptr<qsoft::Texture> getBuffer();

private:
  static std::shared_ptr<Window> initialize();

  std::sr1::zero_initialized<int> width;
  std::sr1::zero_initialized<int> height;

  std::weak_ptr<Camera> currentCamera;

  std::shared_ptr<qsoft::Texture> buffer;
  std::shared_ptr<qsoft::Renderer> renderer;

};
