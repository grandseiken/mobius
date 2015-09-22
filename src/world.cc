#include "world.h"
#include "mesh.h"
#include "proto_util.h"
#include "render.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {
  glm::mat4 orientation_matrix(const Orientation& orientation, bool direction)
  {
    auto target = direction ?
        orientation.origin + orientation.normal :
        orientation.origin - orientation.normal;
    return glm::lookAt(orientation.origin, target, orientation.up);
  }

  glm::mat4 portal_matrix(const Portal& portal)
  {
    auto local = orientation_matrix(portal.local, false);
    auto remote = orientation_matrix(portal.remote, true);
    return glm::inverse(local) * remote;
  }

  std::vector<std::pair<glm::vec3, glm::vec3>>
  calculate_frustum_data(const Player& player, float aspect_ratio)
  {
    auto eye = player.get_head_position();
    auto dir = player.get_look_direction();
    auto f = std::tan(player.get_fov() / 2);

    auto h_normal = glm::normalize(glm::cross(dir, glm::vec3{0, 1, 0}));
    auto v_normal = glm::normalize(glm::cross(h_normal, dir));
    auto h = f * aspect_ratio * h_normal;
    auto v = f * v_normal;

    auto b = eye + dir - v;
    auto t = eye + dir + v;
    auto l = eye + dir - h;
    auto r = eye + dir + h;

    auto bn = glm::normalize(glm::cross(b - eye, b - h - eye));
    auto tn = glm::normalize(glm::cross(t - eye, t + h - eye));
    auto ln = glm::normalize(glm::cross(l - eye, t - h - eye));
    auto rn = glm::normalize(glm::cross(r - eye, b + h - eye));

    std::vector<std::pair<glm::vec3, glm::vec3>> result;
    result.push_back({t, tn});
    result.push_back({b, bn});
    result.push_back({l, ln});
    result.push_back({r, rn});
    result.push_back({eye + player.get_z_near() * dir, dir});
    result.push_back({eye + player.get_z_far() * dir, -dir});
    return result;
  }

  bool mesh_visible(const std::vector<std::pair<glm::vec3, glm::vec3>>& planes,
                    const glm::vec3& eye,
                    const glm::mat4& transform, const Mesh& mesh)
  {
    auto check = [&](const glm::vec3& point, const glm::vec3& normal,
                     const glm::vec3& v)
    {
      return glm::dot(v - point, normal) >= 0;
    };

    auto inside_all = [&](const glm::vec3& v)
    {
      for (const auto& plane: planes) {
        if (!check(plane.first, plane.second, v)) {
          return false;
        }
      }
      return true;
    };

    auto outside_any = [&](
        const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
      for (const auto& plane: planes) {
        if (!check(plane.first, plane.second, a) &&
            !check(plane.first, plane.second, b) &&
            !check(plane.first, plane.second, c)) {
          return true;
        }
      }
      return false;
    };

    // Simple visibility determination. We will probably need something more
    // robust.
    for (const auto& v : mesh.physical_faces()) {
      glm::vec3 a{transform * glm::vec4{v.a, 1}};
      glm::vec3 b{transform * glm::vec4{v.b, 1}};
      glm::vec3 c{transform * glm::vec4{v.c, 1}};

      // Back face cull.
      auto normal = glm::cross(b - a, c - a);
      if (glm::dot(normal, eye - a) <= 0) {
        continue;
      }

      // (Conservative) view frustum intersection.
      if (inside_all(a) || inside_all(b) || inside_all(c) ||
          !outside_any(a, b, c)) {
        return true;
      }
    }

    return false;
  }
}

World::World(const std::string& path, Renderer& renderer)
: _renderer(renderer)
, _player{_collision, {0, 1, 0}, glm::pi<float>() / 2, 1. / 256, 256}
{
  auto world = load_proto<mobius::proto::world>(path);
  for (const auto& chunk_proto : world.chunk()) {
    if (_active_chunk.empty()) {
      _active_chunk = chunk_proto.name();
    }

    Chunk& chunk = _chunks[chunk_proto.name()];
    chunk.mesh.reset(new Mesh{chunk_proto.mesh()});
    for (const auto& portal_proto : chunk_proto.portal()) {
      chunk.portals.emplace_back();
      auto& portal = *chunk.portals.rbegin();
      portal.chunk_name = portal_proto.chunk_name(),
      portal.portal_id = portal_proto.portal_id();
      portal.portal_mesh.reset(new Mesh{portal_proto.portal_mesh()}),

      portal.local.origin = load_vec3(portal_proto.local().origin());
      portal.local.normal = load_vec3(portal_proto.local().normal()),
      portal.local.up = load_vec3(portal_proto.local().up());

      portal.remote.origin = load_vec3(portal_proto.remote().origin()),
      portal.remote.normal = load_vec3(portal_proto.remote().normal());
      portal.remote.up = load_vec3(portal_proto.remote().up());
    }
  }
}

void World::update(const ControlData& controls)
{
  auto it = _chunks.find(_active_chunk);
  if (it == _chunks.end()) {
    return;
  }

  std::vector<Object> environment;
  environment.push_back({it->second.mesh.get(), _orientation});
  for (const auto& portal : it->second.portals) {
    auto jt = _chunks.find(portal.chunk_name);
    if (jt == _chunks.end()) {
      continue;
    }
    // The order looks wrong, but: we want to premultiply by
    //   orientation * portal_matrix * orientation^(-1)
    // which is the same as postmultiplying by portal_matrix.
    environment.push_back(
        {jt->second.mesh.get(), _orientation * portal_matrix(portal)});
  }

  auto player_origin = _player.get_position();
  _player.update(controls, environment);
  auto player_move = _player.get_position() - player_origin;
  for (const auto& portal : it->second.portals) {
    Object object{portal.portal_mesh.get(), _orientation};
    // For the same reasons as general collision, we need to consider several
    // vertices of the object to avoid it slipping through quads.
    //
    // The scale factor should be large enough such that the scaled width of
    // the player mesh is less than the distance between corresponding portal
    // meshes, but small enough that the projection quad never touches the
    // portal stencil.
    bool crossed = false;
    for (const auto& v : _player.get_mesh().physical_vertices()) {
      const float scale_factor = .5f;
      auto point = scale_factor * v + player_origin;
      if (_collision.intersection(point, player_move, object)) {
        crossed = true;
        break;
      }
    }
    if (!crossed) {
      continue;
    }

    // We probably want to translate back to the origin at some point (without
    // messing with normals, somehow).
    _active_chunk = portal.chunk_name;
    _orientation = _orientation * portal_matrix(portal);
    break;
  }
}

void World::render() const
{
  auto it = _chunks.find(_active_chunk);
  if (it == _chunks.end()) {
    return;
  }

  static const uint32_t max_iterations = 4;
  auto inv_orientation = glm::inverse(_orientation);
  auto head = _player.get_head_position();
  auto look_at = head + _player.get_look_direction();
  _renderer.perspective(
      _player.get_fov(), _player.get_z_near(), _player.get_z_far());
  _renderer.camera(head, look_at, {0, 1, 0});
  _renderer.light(head, 128.f);
  _renderer.clear();

  struct chunk_entry {
    const Chunk* chunk;
    const Portal* source;
    glm::mat4 orientation;
    glm::vec3 clip_point;
    glm::vec3 clip_normal;
    uint32_t stencil;
  };

  auto combine_mask = [&](uint32_t iteration,
                          uint32_t test_mask, uint32_t write_mask)
  {
    return iteration % 2 ?
        (test_mask & 0x0f) | (write_mask & 0x0f) << 4 :
        (write_mask & 0x0f) | (test_mask & 0x0f) << 4;
  };

  auto clip_planes =
      calculate_frustum_data(_player, _renderer.get_aspect_ratio());

  std::vector<chunk_entry> buffer_a;
  std::vector<chunk_entry> buffer_b;
  buffer_a.push_back(
      chunk_entry{&it->second, nullptr, _orientation, {}, {}, 0});

  const std::vector<chunk_entry>* read_buffer = &buffer_a;
  std::vector<chunk_entry>* write_buffer = &buffer_b;

  for (std::size_t iteration = 0;
       iteration < max_iterations && !read_buffer->empty(); ++iteration) {
    bool last_iteration = iteration + 1 >= max_iterations;
    uint32_t iteration_stencil = 0;
    uint32_t stencil_test_mask =
        !iteration ? 0x00 : combine_mask(iteration, 0xf, 0x0);
    uint32_t stencil_write_mask = combine_mask(iteration, 0x0, 0xf);

    if (iteration == 1) {
      clip_planes.emplace_back();
    }
    if (iteration >= 2 && !last_iteration) {
      _renderer.clear_stencil(stencil_write_mask);
    }

    for (const auto& entry : *read_buffer) {
      if (iteration) {
        clip_planes.rbegin()->first = entry.clip_point;
        clip_planes.rbegin()->second = entry.clip_normal;
      }

      uint32_t stencil_ref = combine_mask(iteration, entry.stencil, 0x0);
      _renderer.world(entry.orientation, entry.clip_point, entry.clip_normal);
      _renderer.depth(*entry.chunk->mesh, stencil_ref, stencil_test_mask);
      _renderer.draw(*entry.chunk->mesh, stencil_ref, stencil_test_mask);

      for (const auto& portal : entry.chunk->portals) {
        auto jt = _chunks.find(portal.chunk_name);
        bool is_source = entry.source &&
            portal.portal_id == entry.source->portal_id &&
            &portal != entry.source;

        if (last_iteration || jt == _chunks.end() || is_source ||
            !mesh_visible(clip_planes, head,
                          entry.orientation, *portal.portal_mesh)) {
          continue;
        }

        // To avoid awkwardly-placed portals being seen through other stencils
        // and messing up the buffer, we have to render first with depth-writing
        // enabled and then manually clear the depth. (A possibly-faster
        // approach might be to divide up the depth buffer according to
        // max_iterations and use different ranges for each level of the portal
        // tree. This might also avoid having to clear the stencil buffer?)
        // TODO: this should really warn when we reuse stencil bits.
        auto stencil = 1 + iteration_stencil++ % (0xf - 1);
        auto stencil_ref = combine_mask(iteration, entry.stencil, stencil);
        _renderer.stencil(*portal.portal_mesh, stencil_ref,
                          stencil_test_mask, stencil_write_mask);

        // We have to clip behind the portal so that we don't see overlapping
        // geometry hanging about.
        auto normal_orientation =
            glm::transpose(glm::inverse(glm::mat3{entry.orientation}));
        glm::vec3 clip_point{
            entry.orientation * glm::vec4{portal.local.origin, 1}};
        auto clip_normal = -normal_orientation * portal.local.normal;

        auto orientation = entry.orientation * portal_matrix(portal);
        write_buffer->push_back(chunk_entry{&jt->second, &portal, orientation,
                                            clip_point, clip_normal, stencil});
      }
    }

    // Clear all the portal depths for the next iteration.
    for (const auto& entry : *write_buffer) {
      auto next_stencil_ref = combine_mask(iteration + 1, entry.stencil, 0x0);
      auto next_stencil_test_mask = combine_mask(iteration + 1, 0xf, 0x0);
      _renderer.depth_clear(*entry.source->portal_mesh,
                            next_stencil_ref, next_stencil_test_mask);
    }

    auto translate = glm::translate(glm::mat4{}, _player.get_position());
    // Render objects in each chunk.
    // TODO: isn't right any more, but drawing the objects from the next
    // iteration in this stencil could result in overlapping spaces. Need to
    // think it through.
    for (const auto& entry : *read_buffer) {
      auto transform = entry.orientation * inv_orientation * translate;
      uint32_t stencil_ref = combine_mask(iteration, entry.stencil, 0x0);

      // Since we currently only have a player, "get objects in chunk" is just
      // "get the player in the active_chunk on nonzero iterations".
      if (iteration && entry.chunk == &it->second) {
        _renderer.world(transform);
        _renderer.draw(
            _player.get_mesh(), stencil_ref, stencil_test_mask);
      }
    }

    read_buffer = iteration % 2 ? &buffer_a : &buffer_b;
    write_buffer = iteration % 2 ? &buffer_b : &buffer_a;
    write_buffer->clear();
  }
  _renderer.grain(1. / 32);
  _renderer.render();
}
