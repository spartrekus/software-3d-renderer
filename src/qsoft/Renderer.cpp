#include "Renderer.h"
#include "Mesh.h"
#include "Texture.h"
#include "Matrix.h"
#include "Face.h"
#include "Color.h"

// TODO:
#include "Debug.h"
#include <iostream>
#include <algorithm>

#include <limits>
#include <vector>

#include <cmath>

namespace qsoft
{

struct RendererImpl
{
  Mesh mesh;
  Texture texture;
  Matrix view;
  Matrix model;
  Matrix projection;
  Texture target;
  Matrix viewport;

  std::vector<Vertex> vertices;
  std::vector<Vertex> aux;

  void clippedTriangle(const Vertex& a, const Vertex& b, const Vertex& c);
  void triangle(const Vertex& a, const Vertex& b, const Vertex& c);
  bool insideViewFrustrum(const Vertex& v);
  Vector3 barycentric(Vector3& a, Vector3& b, Vector3& c, Vector2& p);
  void clipPolyComponent(std::vector<Vertex>& vertices, int comp, float fac, std::vector<Vertex>& result);
  bool clipPolyAxis(std::vector<Vertex>& vertices, std::vector<Vertex>& aux, int comp);
};

Renderer::Renderer() : impl(std::make_shared<RendererImpl>())
{

}

void Renderer::setMesh(const Mesh& mesh)
{
  impl->mesh = mesh;
}

void Renderer::setTexture(const Texture& texture)
{
  impl->texture = texture;
}

void Renderer::setTarget(const Texture& target)
{
  impl->target = target;
  impl->viewport = Matrix::viewport(0, 0, target.getWidth(), target.getHeight());
  //impl->viewport = Matrix::viewport(0, 0, target.getWidth(), target.getHeight() - 100);
}

void Renderer::setView(const Matrix& view)
{
  impl->view = view;
}

void Renderer::setModel(const Matrix& model)
{
  impl->model = model;
}

void Renderer::setProjection(const Matrix& projection)
{
  impl->projection = projection;
}

bool RendererImpl::insideViewFrustrum(const Vertex& v)
{
  return fabs(v.position.x) <= fabs(v.position.w) &&
    fabs(v.position.y) <= fabs(v.position.w) &&
    fabs(v.position.z) <= fabs(v.position.w);
}

Vector3 RendererImpl::barycentric(Vector3& a, Vector3& b, Vector3& c, Vector2& p)
{
  Vector3 s0 = Vector3(c.x - a.x, b.x - a.x, a.x - p.x);
  Vector3 s1 = Vector3(c.y - a.y, b.y - a.y, a.y - p.y);

  Vector3 u = Vector3::cross(s0, s1);

  Vector3 r(-1, 1, 1);

  if(fabs(u.z) < 1.0f)
  {
    return r;
  }

  r.x = 1.0f - (u.x + u.y) / u.z;
  r.y = u.y / u.z;
  r.z = u.x / u.z;

  return r;
}

bool RendererImpl::clipPolyAxis(std::vector<Vertex>& vertices, std::vector<Vertex>& aux, int comp)
{
  clipPolyComponent(vertices, comp, 1.0f, aux);
  vertices.clear();

  if(aux.size() == 0)
  {
    return false;
  }

  clipPolyComponent(aux, comp, -1.0f, vertices);
  aux.clear();

  return vertices.size() != 0;
}

void RendererImpl::clipPolyComponent(std::vector<Vertex>& vertices, int comp, float fac, std::vector<Vertex>& result)
{
  Vertex previousVertex = vertices.at(vertices.size() - 1);
  float previousComp = previousVertex.getComponent(comp) * fac;
  bool previousInside = previousComp <= previousVertex.position.w;

  for(int i = 0; i < vertices.size(); i++)
  {
    Vertex& currentVertex = vertices.at(i);
    float currentComp = currentVertex.getComponent(comp) * fac;
    bool currentInside = currentComp <= currentVertex.position.w;

    if(currentInside ^ previousInside)
    {
      float wMB = previousVertex.position.w - previousComp;
      float lerpAmt = wMB / (wMB - (currentVertex.position.w - currentComp));
      Vertex lv = Vertex::lerp(previousVertex, currentVertex, lerpAmt);
      result.push_back(lv);
    }

    if(currentInside)
    {
      result.push_back(currentVertex);
    }

    previousComp = currentComp;
    previousInside = currentInside;
    previousVertex = currentVertex;
  }
}

void RendererImpl::triangle(const Vertex& a, const Vertex& b, const Vertex& c)
{
  Vector2 bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
  Vector2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
  Vector2 clamp(target.getWidth() - 1, target.getHeight() - 1);

  Vertex pts[3] =
  {
    a.transform(viewport),
    b.transform(viewport),
    c.transform(viewport)
  };

  Vector3 pts2[3];

  for(int i = 0; i < 3; i++)
  {
    pts2[i].x = pts[i].position.x / pts[i].position.w;
    pts2[i].y = pts[i].position.y / pts[i].position.w;
    pts2[i].z = pts[i].position.z / pts[i].position.w;
  }

  Vector3 av = Vector3(pts2[0].x, pts2[0].y, pts2[0].z);
  Vector3 bv = Vector3(pts2[1].x, pts2[1].y, pts2[1].z);
  Vector3 cv = Vector3(pts2[2].x, pts2[2].y, pts2[2].z);
  Vector3 N = Vector3::cross((av - bv), (bv - cv));
  if(N.z > 0) return;

  for(int i = 0; i < 3; i++)
  {
    bboxmin.x = std::max(0.0f, std::min(bboxmin.x, pts2[i].x));
    bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts2[i].x));
    bboxmin.y = std::max(0.0f, std::min(bboxmin.y, pts2[i].y));
    bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts2[i].y));
  }

  float sz = (float)std::max(target.getWidth(), target.getHeight());
  float bcsoff = -0.01f / sz;
  int cp = bboxmax.y + 1;

  #pragma omp parallel
  #pragma omp for
  for(int py = bboxmin.y; py < cp; ++py)
  {
    for(int px = bboxmin.x; px <= bboxmax.x; ++px)
    {
      Vector2 P(px, py);
      Vector3 bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);

      if(bc_screen.x < bcsoff || bc_screen.y < bcsoff || bc_screen.z < bcsoff)
      {
        continue;
      }

      Vector3 bc_clip(
        bc_screen.x / pts[0].position.w,
        bc_screen.y / pts[1].position.w,
        bc_screen.z / pts[2].position.w
      );

      float depth = bc_clip.x + bc_clip.y + bc_clip.z;
      bool cont = true;
      float lastDepth = 0;

      #pragma omp critical
      {
        lastDepth = target.getDepth(px, py);

        if(lastDepth < depth)
        {
          target.setDepth(px, py, depth);
          cont = false;
        }
      }

      if(cont) continue;

      float u = (a.texCoord.x * bc_clip.x + b.texCoord.x * bc_clip.y + c.texCoord.x * bc_clip.z) /
        (bc_clip.x + bc_clip.y + bc_clip.z);

      float v = 1.0f - (a.texCoord.y * bc_clip.x + b.texCoord.y * bc_clip.y + c.texCoord.y * bc_clip.z) /
        (bc_clip.x + bc_clip.y + bc_clip.z);

      while(u < 0.0f)
      {
        u += 1.0f;
      }

      while(u > 1.0f)
      {
        u -= 1.0f;
      }

      while(v < 0.0f)
      {
        v += 1.0f;
      }

      while(v > 1.0f)
      {
        v -= 1.0f;
      }

      int tx = (int)(u * (float)(texture.getWidth() - 1) + 0.5f);
      int ty = (int)(v * (float)(texture.getHeight() - 1) + 0.5f);
      Color frag = texture.getPixel(tx, ty);

      if(frag.a == 0)
      {
        target.setDepth(px, py, lastDepth);
        continue;
      }

      //int tx = (int)(u * (float)texture.getWidth());
      //int ty = (int)(v * (float)texture.getHeight());
      target.setPixel(px, py, frag);

      // TODO: If !texture?
      // target.setPixel(px, py, Color(0, 255, 0));
      //target.setPixel(px, py, Color(v * 255, 255, 255));
    }
  }
}

void RendererImpl::clippedTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
{
  if(insideViewFrustrum(a) &&
    insideViewFrustrum(b) &&
    insideViewFrustrum(c))
  {
    triangle(a, b, c);
    // TODO: I should return?
    return;
  }

  vertices.clear();
  aux.clear();

  vertices.push_back(a);
  vertices.push_back(b);
  vertices.push_back(c);

  if(clipPolyAxis(vertices, aux, 0) &&
    clipPolyAxis(vertices, aux, 1) &&
    clipPolyAxis(vertices, aux, 2))
  {
    Vertex& initial = vertices.at(0);

    for(int i = 1; i < vertices.size() - 1; i++)
    {
      Vertex& v1 = vertices.at(i);
      Vertex& v2 = vertices.at(i + 1);
      triangle(initial, v1, v2);
    }
  }
}

void Renderer::render()
{
  Matrix m = impl->projection * impl->view * impl->model;

  for(std::vector<Face>::iterator it = impl->mesh.getFaces().begin();
    it != impl->mesh.getFaces().end(); it++)
  {
    impl->clippedTriangle(
      it->a.transform(m),
      it->b.transform(m),
      it->c.transform(m));
  }
}

}
