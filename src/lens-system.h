#ifndef _LENS_SYSTEM_H
#define _LENS_SYSTEM_H

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
using JSON = nlohmann::json;

#include "lens-element.h"

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

class LensSystem {
 public:
  std::vector<std::shared_ptr<LensElement>> elements;

  Real object_focal_z;
  Real object_principal_z;
  Real object_focal_length;
  Real image_focal_z;
  Real image_principal_z;
  Real image_focal_length;

  LensSystem(const std::string& filename) {
    // load json
    if (!loadJSON(filename)) exit(EXIT_FAILURE);

    // compute system length and z
    Real length = 0;
    for (auto itr = elements.rbegin(); itr != elements.rend(); itr++) {
      length += (*itr)->thickness;
      (*itr)->z = -length;
    }

    // compute cardinal points
    if (!computeCardinalPoints()) exit(EXIT_FAILURE);
  }

  bool loadJSON(const std::string& filename) {
    // open file
    std::ifstream stream(filename);
    if (!stream) {
      std::cerr << "failed to open " << filename << std::endl;
      return false;
    }

    // parse JSON
    JSON json;
    stream >> json;

    // push lens elements
    for (const auto& [key, value] : json.items()) {
      // unit conversion from [mm] to [m]
      const unsigned int index = value["index"].get<unsigned int>();
      const Real curvature_radius =
          value["curvature_radius"].get<Real>() * 1e-3f;
      const Real thickness = value["thickness"].get<Real>() * 1e-3f;
      const Real ior = value["eta"].get<Real>();
      const Real aperture_radius =
          0.5f * value["aperture_diameter"].get<Real>() * 1e-3f;

      // Aperture
      if (curvature_radius == 0.0f) {
        auto element =
            std::make_shared<Aperture>(index, aperture_radius, thickness);
        elements.push_back(element);
      }
      // Lens
      else {
        auto element = std::make_shared<Lens>(index, aperture_radius, thickness,
                                              curvature_radius, ior);
        elements.push_back(element);
      }
    }

    // sort lens elements by index
    std::sort(elements.begin(), elements.end(),
              [](const std::shared_ptr<LensElement>& x1,
                 const std::shared_ptr<LensElement>& x2) {
                return x1->index < x2->index;
              });

    return true;
  };

  bool raytrace(const Ray& ray_in, Ray& ray_out,
                bool reflection = false) const {
    int element_index = ray_in.direction.z() > 0 ? -1 : elements.size();
    Ray ray = ray_in;
    Real ior = 1.0f;

    while (true) {
      // update element index
      element_index += ray.direction.z() > 0 ? 1 : -1;
      if (element_index < 0 || element_index >= elements.size()) break;
      const auto element = elements[element_index];

      // Aperture
      if (const std::shared_ptr<Aperture> aperture =
              std::dynamic_pointer_cast<Aperture>(element)) {
        Hit res;
        if (!aperture->intersect(ray, res)) return false;

        // Update ray
        ray = Ray(res.hitPos, ray.direction);

        // Update ior
        ior = 1.0f;
      }
      // Lens
      else if (const std::shared_ptr<Lens> lens =
                   std::dynamic_pointer_cast<Lens>(element)) {
        // Compute Next Element IOR
        Real next_ior = 1.0f;

        // Compute Next Element
        const int next_element_index =
            ray.direction.z() > 0 ? element_index : element_index - 1;
        if (next_element_index >= 0) {
          const std::shared_ptr<LensElement> next_element =
              elements[next_element_index];
          if (const std::shared_ptr<Lens> next_lens =
                  std::dynamic_pointer_cast<Lens>(next_element)) {
            next_ior = next_lens->ior;
          }
        }

        // Compute Intersection with Lens
        Hit res;
        if (!lens->intersect(ray, res)) return false;

        // Refract and Reflect
        if (reflection) {
          // TODO: implement this
        } else {
          Vec3 next_direction;
          if (!refract(-ray.direction, next_direction, res.hitNormal, ior,
                       next_ior))
            return false;

          // Set Next Ray
          ray = Ray(res.hitPos, next_direction);

          // update ior
          ior = next_ior;
        }
      } else {
        std::cerr << "invalid lens element" << std::endl;
        return false;
      }
    }

    ray_out = ray;

    return true;
  }

  bool computeCardinalPoints() {
    const Real height = 0.001f;

    // raytrace from object plane
    Ray ray_in(Vec3(0, height, elements.front()->z - 1.0f), Vec3(0, 0, 1));
    Ray ray_out;
    if (!raytrace(ray_in, ray_out)) {
      std::cerr << "failed to compute cardinal points" << std::endl;
      return false;
    }

    // compute image focal point
    Real t = -ray_out.origin.y() / ray_out.direction.y();
    image_focal_z = ray_out(t).z();

    // compute image principal point
    t = -(ray_out.origin.y() - height) / ray_out.direction.y();
    image_principal_z = ray_out(t).z();

    // compute image focal length
    image_focal_length = image_focal_z - image_principal_z;

    // raytrace from image plane
    ray_in = Ray(Vec3(0, height, 0), Vec3(0, 0, -1));
    if (!raytrace(ray_in, ray_out)) {
      std::cerr << "failed to compute cardinal points" << std::endl;
      return false;
    }

    // compute object focal point
    t = -ray_out.origin.y() / ray_out.direction.y();
    object_focal_z = ray_out(t).z();

    // compute object principal point
    t = -(ray_out.origin.y() - height) / ray_out.direction.y();
    object_principal_z = ray_out(t).z();

    // compute object focal length
    object_focal_length = object_focal_z - object_principal_z;

    return true;
  }
};

#endif