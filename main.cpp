#include "defs.h"
#include <optional>
#include <png++/png.hpp>
#include <utility>
#include <vector>
#include <chrono> 


int Cw = 2000;
int Ch = 2000;
int Vw = 1;
int Vh = 1;
int z_dist = 1;
/**
 * Definitions of the scene.
 * This is what is actually being rendered.
 */
struct Scene {
  Point camera;
  std::vector<Sphere> spheres;
  std::vector<Light> lights;
  Scene() {
    camera = Point(0, 0, 0);
    spheres.emplace_back(Point(0, -1, 3), 1, RED);
    spheres.emplace_back(Point(2, 0, 4), 1, GREEN);
    spheres.emplace_back(Point(-2, 0, 4), 1, BLUE);
    spheres.emplace_back(Point(0, -5001, 0), 5000, Color(0, 255, 255)); // Teal?
    lights.emplace_back(Point(0, 0, 0), .2, AMBIENT);
    lights.emplace_back(Point(2, 1, 0), .6, POINT);
    lights.emplace_back(Point(1, 4, 4), .2, DIRECTIONAL);
    std::cout << "Initialized scene with " << camera << " camera" << std::endl
              << spheres.size() << " spheres" << std::endl
              << lights.size() << " lights." << std::endl
              << std::endl;
  }
};
Scene scene;

/**
 * Solves the quadratic formula of where a ray intersects the surface of a
 * sphere.
 *
 * Derivation comes from:
 *
 * `
 * radius^2 = |P - center| * |P - center| => <P-center, P-center>
 * P = origin + dir*t
 * `
 *
 * Plugging p into radius eq and solving for t
 * gives a value t that equals points on surface of the sphere.
 */
std::pair<double, double> intersect_ray_sphere(Point &origin, Point &dir,
                                               Sphere &sphere) {
  auto oc = origin - sphere.center;

  auto k1 = dir * dir;
  auto k2 = 2.0 * (oc * dir);
  auto k3 = oc * oc - sphere.radius * sphere.radius;

  auto discriminant = k2 * k2 - 4.0 * k1 * k3;
  if (discriminant < 0.0) {
    return std::make_pair(DBL_MAX, DBL_MAX);
  } else {
    auto t1 = (-k2 + std::sqrt(discriminant)) / (2.0 * k1);
    auto t2 = (-k2 - std::sqrt(discriminant)) / (2.0 * k1);
    return std::make_pair(t1, t2);
  }
}

/**
 * Computation of the light vectors is done by computing
 * normal vectors of the surface of the sphere and then
 * taking the dot product
 * with the direction from the light source.
 * Depending on light source type, adjust vector as appropriate.
 */
double compute_lighting(Point P, Point N) {
  auto i = 0.0;
  for (auto &l : scene.lights) {
    if (l.type == AMBIENT) {
      i += l.intensity;
    } else {
      Point L;
      if (l.type == POINT) {
        L = l.vector - P;
      } else if (l.type == DIRECTIONAL) {
        L = l.vector;
      }
      double dot = N * L;
      if (dot > 0) {
        i += (l.intensity * dot) / (N.length() * L.length());
      }
    }
  }
  return i;
}

/**
 * Simulate a single ray from point extending outwards in dir.
 * Returns the color for the specific square.
 * This is the algorithm that outputs the colors
 * necessary to create the image.
 */
Color trace_ray(Point &origin, Point &dir, double t_min, double t_max) {
  auto t = DBL_MAX;
  std::optional<Sphere> m;
  for (auto &s : scene.spheres) {
    auto t_s = intersect_ray_sphere(origin, dir, s);
    auto fst = t_s.first;
    auto snd = t_s.second;
    if (fst < t && t_min < fst && fst < t_max) {
      t = fst;
      m = s;
    }
    if (snd < t && t_min < snd && snd < t_max) {
      t = snd;
      m = s;
    }
  }
  if (m) {
    auto P = origin + (dir * t);
    auto N = P - m.value().center;
    N = N / N.length();
    return m.value().color * compute_lighting(P, N);
  }
  return WHITE;
}

/**
 * Create the canvas, a scaling function,
 * then iterate over each pixel
 * and trace the path of ray from camera towards the viewport.
 */
int main() {
  using std::chrono::system_clock;
  using std::chrono::milliseconds;
  using std::chrono::duration_cast;

  png::image<png::rgb_pixel> image(Cw, Ch);
  auto canvas_to_viewport = [&](double x, double y) {
    return Point(x * Vw / Cw, y * Vh / Ch, z_dist);
  };
  auto CwO = Cw / 2;
  auto ChO = Ch / 2;
  auto start = system_clock::now();
  for (int x = -Cw / 2; x < Cw / 2; x++) {
    for (int y = -Ch / 2; y < Ch / 2; y++) {
      auto dir = canvas_to_viewport(x, y);
      auto color = trace_ray(scene.camera, dir, 0, 2000);
      image[ChO - y - 1][x + CwO] = png::rgb_pixel(color.r, color.g, color.b);
    }
  }
  auto stop = system_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  std::cout << "Took " << duration.count() << " ms.";
  image.write("test.png");
  return 0;
}
