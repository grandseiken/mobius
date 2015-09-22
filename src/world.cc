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
      for (const auto& plane : planes) {
        if (!check(plane.first, plane.second, v)) {
          return false;
        }
      }
      return true;
    };

    auto outside_any = [&](
        const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
      for (const auto& plane : planes) {
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
    // The scale factor should be small enough such that the scaled width of
    // the player mesh is less than the distance between corresponding portal
    // meshes, but large enough that the projection quad never touches the
    // portal stencil before we change chunks.
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

  struct world_data {
    glm::mat4 orientation;
    glm::vec3 clip_point;
    glm::vec3 clip_normal;
  };

  struct chunk_entry {
    const Chunk* chunk;
    const Portal* source;
    uint32_t stencil;

    world_data data;
    world_data source_data;
  };

  static const uint32_t value_bits = 0x7f;
  static const uint32_t flag_bits = 0x80;
  auto combine_mask = [&](bool flag, uint32_t value)
  {
    return (flag ? flag_bits : 0) | (value & value_bits);
  };
  auto clip_planes =
      calculate_frustum_data(_player, _renderer.get_aspect_ratio());
  // Space for the portal clip plane.
  clip_planes.emplace_back();

  std::vector<chunk_entry> buffer_a;
  std::vector<chunk_entry> buffer_b;
  buffer_a.push_back({&it->second, nullptr, 0,
                      {_orientation, {}, {}}, {{}, {}, {}}});

  const std::vector<chunk_entry>* read_buffer = &buffer_a;
  std::vector<chunk_entry>* write_buffer = &buffer_b;

  for (std::size_t iteration = 0;
       iteration < max_iterations && !read_buffer->empty(); ++iteration) {
    bool last_iteration = iteration + 1 >= max_iterations;
    uint32_t iteration_stencil = 0;

    for (const auto& entry : *read_buffer) {
      if (iteration) {
        clip_planes.rbegin()->first = entry.data.clip_point;
        clip_planes.rbegin()->second = entry.data.clip_normal;
      }

      // Establish depth buffer for this chunk.
      uint32_t stencil_ref = combine_mask(false, entry.stencil);
      _renderer.world(entry.data.orientation,
                      entry.data.clip_point, entry.data.clip_normal);
      _renderer.depth(*entry.chunk->mesh, stencil_ref, value_bits);

      // Render objects in the chunk.
      // Since we currently only have a player, "get objects in chunk" is just
      // "get the player in the active_chunk on nonzero iterations".
      // TODO: isn't right any more, but drawing the objects from the next
      // iteration in this stencil could result in overlapping spaces. Need to
      // think it through.
      if (iteration && entry.chunk == &it->second) {
        auto translate = glm::translate(glm::mat4{}, _player.get_position());
        _renderer.world(entry.data.orientation * inv_orientation * translate);
        _renderer.draw(_player.get_mesh(), stencil_ref, value_bits);
      }

      // Renderer the chunk.
      _renderer.world(entry.data.orientation,
                      entry.data.clip_point, entry.data.clip_normal);
      _renderer.draw(*entry.chunk->mesh, stencil_ref, value_bits);

      // To avoid awkwardly-placed portals being seen through other stencils
      // and messing up the buffer, and maximise the individual bit-
      // combinations we can use to represent different portals per iteration,
      // we do the following:
      // (1) render a single flag bit into the stencil buffer for all portals,
      //     with depth enabled
      // (2) clear the value bits
      // (3) rerender one of (127 - 1) remaining combinations of stencil bits
      //     to the flagged areas only with depth function GL_EQUAL
      // (4) clear depth and flag bit.
      for (const auto& portal : entry.chunk->portals) {
        auto jt = _chunks.find(portal.chunk_name);
        bool is_source = entry.source &&
            portal.portal_id == entry.source->portal_id &&
            &portal != entry.source;

        if (last_iteration || jt == _chunks.end() || is_source ||
            !mesh_visible(clip_planes, head,
                          entry.data.orientation, *portal.portal_mesh)) {
          continue;
        }
        // TODO: this should really warn when we reuse stencil bits.
        auto stencil = 1 + iteration_stencil++ % (value_bits - 1);

        // We have to clip behind the portal so that we don't see overlapping
        // geometry hanging about.
        auto normal_orientation =
            glm::transpose(glm::inverse(glm::mat3{entry.data.orientation}));
        glm::vec3 clip_point{
            entry.data.orientation * glm::vec4{portal.local.origin, 1}};
        auto clip_normal = -normal_orientation * portal.local.normal;

        auto orientation = entry.data.orientation * portal_matrix(portal);
        write_buffer->push_back({&jt->second, &portal, stencil,
                                 {orientation, clip_point, clip_normal},
                                 entry.data});

        auto portal_stencil_ref = combine_mask(true, entry.stencil);
        _renderer.stencil(
            *portal.portal_mesh, portal_stencil_ref,
            /* read */ value_bits, /* write */ flag_bits, /* depth_eq */ false);
      }
    }

    _renderer.clear_stencil(value_bits);
    // This part could theoretically cause artifacts when portals visible
    // through different portals intersect exactly in camera space.
    // TODO: is there any way to avoid it? Possibly by using additional
    // clipping planes just for this bit, since such portals would necessarily
    // be separated by the portals they're being viewed through. We probably
    // want to use such clipping planes anyway!
    for (const auto& entry : *write_buffer) {
      auto portal_stencil_ref = combine_mask(true, entry.stencil);
      _renderer.world(entry.source_data.orientation,
                      entry.source_data.clip_point,
                      entry.source_data.clip_normal);
      _renderer.stencil(
          *entry.source->portal_mesh, portal_stencil_ref,
          /* read */ flag_bits, /* write */ value_bits, /* depth_eq */ true);
    }

    // Clear all the portal depths for the next iteration and swap buffers.
    _renderer.clear_depth();
    _renderer.clear_stencil(flag_bits);
    read_buffer = iteration % 2 ? &buffer_a : &buffer_b;
    write_buffer = iteration % 2 ? &buffer_b : &buffer_a;
    write_buffer->clear();
  }
  _renderer.grain(1. / 32);
  _renderer.render();
}
