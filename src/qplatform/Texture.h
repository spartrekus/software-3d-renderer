#include "Resource.h"

#include <qsoft/qsoft.h>

#include <memory>

class ModelRenderer;
class SpriteRenderer;

class Texture : public Resource
{
  friend class ModelRenderer;
  friend class SpriteRenderer;

  std::shared_ptr<qsoft::Texture> data;

  void onLoad(std::string path);

public:
  void onCreate(int width, int height);

  int getWidth();
  int getHeight();
  qsoft::Color getPixel(int x, int y);
  void setPixel(int x, int y, const qsoft::Color& color);

};
