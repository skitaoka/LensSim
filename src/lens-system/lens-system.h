#ifndef _LENS_SYSTEM_H
#define _LENS_SYSTEM_H

#include "core/bounds2.h"
#include "film.h"
#include "lens-system/lens-element.h"

using namespace Prl2;

// 反射ベクトルを返す
inline Vec3 reflect(const Vec3& v, const Vec3& n) {
  return -v + 2 * dot(v, n) * n;
}

// フレネル係数を計算する
inline Real fresnel(const Vec3& wo, const Vec3& n, const Real& n1,
                    const Real& n2) {
  const float f0 = std::pow((n1 - n2) / (n1 + n2), 2.0f);
  return f0 + (1.0f - f0) * std::pow(1.0f - dot(wo, n), 5.0f);
}

// 屈折ベクトルを返す
inline bool refract(const Vec3& wi, Vec3& wt, const Vec3& n, const Real& ior1,
                    const Real& ior2) {
  const Real eta = ior1 / ior2;
  const Real cosThetaI = dot(wi, n);
  const Real sin2ThetaI = std::max(0.0f, 1.0f - cosThetaI * cosThetaI);
  const Real sin2ThetaT = eta * eta * sin2ThetaI;
  if (sin2ThetaT >= 1.0f) return false;
  const Real cosThetaT = std::sqrt(1.0f - sin2ThetaT);
  wt = eta * (-wi) + (eta * cosThetaI - cosThetaT) * n;
  return true;
}

// 点を平面で原点中心に回転する
inline Vec2 rotate2D(const Vec2& p, Real theta) {
  return Vec2(p.x() * std::cos(theta) - p.y() * std::sin(theta),
              p.x() * std::sin(theta) + p.y() * std::cos(theta));
}

class LensSystem {
 public:
  std::shared_ptr<Film> film;

  std::vector<std::shared_ptr<LensElement>> elements;

  Real object_focal_z;
  Real object_principal_z;
  Real object_focal_length;
  Real image_focal_z;
  Real image_principal_z;
  Real image_focal_length;

  static constexpr unsigned int num_exit_pupil_bounds = 64;
  static constexpr unsigned int num_exit_pupil_bounds_samples = 1024;
  std::vector<Bounds2> exit_pupil_bounds;

  LensSystem(const std::string& filename, const std::shared_ptr<Film> _film);

  bool loadJSON(const std::string& filename);

  bool raytrace(const Ray& ray_in, Ray& ray_out, bool reflection = false,
                Sampler* sampler = nullptr) const;

  bool computeCardinalPoints();

  bool focus(Real focus_z);

  Bounds2 computeExitPupilBound(const Vec2& p) const;
  bool computeExitPupilBounds();

  bool sampleRay(Real u, Real v, Real lambda, Sampler& sampler, Ray& ray_out,
                 Real& pdf, bool reflection = false) const;
};

#endif