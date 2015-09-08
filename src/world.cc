#include "world.h"
#include "mesh.h"
#include "proto_util.h"
#include "render.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <deque>

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

  bool mesh_visible(const glm::vec3& head, const glm::vec3& look,
                    const glm::mat4& transform, const Mesh& mesh)
  {
    // Extremely simple visibility determination. We will probably need
    // something more robust. The clip check should respect the view frustum at
    // the very least.
    bool clipped = true;
    for (const auto& v : mesh.physical_vertices()) {
      glm::vec3 vt{transform * glm::vec4{v, 1}};
      if (glm::dot(look - head, vt - head) >= 0) {
        clipped = false;
        break;
      }
    }
    bool backfacing = true;
    for (const auto& t : mesh.physical_faces()) {
      glm::vec3 a{transform * glm::vec4{t.a, 1}};
      glm::vec3 b{transform * glm::vec4{t.b, 1}};
      glm::vec3 c{transform * glm::vec4{t.c, 1}};
      auto normal = glm::cross(b - a, c - a);
      if (glm::dot(normal, head - a) >= 0) {
        backfacing = false;
        break;
      }
    }
    return !clipped && !backfacing;
  }
}

World::World(const std::string& path, Renderer& renderer)
: _renderer(renderer)
, _player{_collision, {0, 1, 0}}
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

  static const uint32_t max_iterations = 8;
  auto inv_orientation = glm::inverse(_orientation);
  auto head = _player.get_head_position();
  auto look = _player.get_look_position();
  _renderer.camera(head, look, {0, 1, 0});
  _renderer.light(head, 2.f);
  _renderer.clear();

  struct chunk_entry {
    const Chunk* chunk;
    const Portal* source;
    glm::mat4 orientation;
    glm::vec3 clip_point;
    glm::vec3 clip_normal;
    uint32_t iteration;
    uint32_t stencil;
  };
  std::deque<chunk_entry> queue;
  std::vector<uint32_t> iteration_stencil;
  auto combine_mask = [&](uint32_t iteration,
                          uint32_t test_mask, uint32_t write_mask)
  {
    return iteration % 2 ?
        (test_mask & 0x0f) | (write_mask & 0x0f) << 4 :
        (write_mask & 0x0f) | (test_mask & 0x0f) << 4;
  };

  queue.push_back(
      chunk_entry{&it->second, nullptr, _orientation, {}, {}, 0, 0});
  while (!queue.empty()) {
    auto entry = queue.front();
    queue.pop_front();

    uint32_t stencil_test_mask = !entry.iteration ? 0x00 :
        combine_mask(entry.iteration, 0xf, 0x0);
    uint32_t stencil_write_mask = combine_mask(entry.iteration, 0x0, 0xf);
    uint32_t stencil_ref = combine_mask(entry.iteration, entry.stencil, 0x0);

    _renderer.world(entry.orientation, entry.clip_point, entry.clip_normal);
    _renderer.depth(*entry.chunk->mesh, stencil_ref, stencil_test_mask);
    _renderer.draw(*entry.chunk->mesh, stencil_ref, stencil_test_mask);

    bool last_iteration = entry.iteration + 1 >= max_iterations;
    if (entry.iteration >= iteration_stencil.size()) {
      iteration_stencil.push_back(0);
      if (entry.iteration >= 2 && !last_iteration) {
        _renderer.clear_stencil(stencil_write_mask);
      }
    }

    std::vector<std::pair<const Portal*, uint32_t>> portals_added;
    for (const auto& portal : entry.chunk->portals) {
      auto jt = _chunks.find(portal.chunk_name);
      bool is_source = entry.source &&
          portal.portal_id == entry.source->portal_id &&
          &portal != entry.source;
      if (last_iteration || jt == _chunks.end() || is_source ||
          !mesh_visible(head, look, entry.orientation, *portal.portal_mesh)) {
        continue;
      }

      // To avoid awkwardly-placed portals being seen through other stencils and
      // messing up the buffer, we have to render first with depth-writing
      // enabled and then manually clear the depth. (A possibly-faster approach
      // might be to divide up the depth buffer according to max_iterations and
      // use different ranges for each level of the portal tree. This might also
      // avoid having to clear the stencil buffer?)
      auto stencil = ++iteration_stencil[entry.iteration];
      auto stencil_ref = combine_mask(entry.iteration, entry.stencil, stencil);
      _renderer.stencil(*portal.portal_mesh, stencil_ref,
                        stencil_test_mask, stencil_write_mask);
      portals_added.push_back(std::make_pair(&portal, stencil));

      // We have to clip behind the portal so that we don't see overlapping
      // geometry hanging about.
      auto normal_orientation =
          glm::transpose(glm::inverse(glm::mat3{entry.orientation}));
      glm::vec3 clip_point{entry.orientation * glm::vec4{portal.local.origin, 1}};
      auto clip_normal = -normal_orientation * portal.local.normal;
      auto orientation = entry.orientation * portal_matrix(portal);

      queue.push_back(chunk_entry{
          &jt->second, &portal, orientation, clip_point, clip_normal,
          entry.iteration + 1, stencil});
    }
    // Clear the portal depth.
    for (const auto& pair: portals_added) {
      auto next_stencil_ref = combine_mask(entry.iteration + 1, pair.second, 0x0);
      auto next_stencil_test_mask = combine_mask(entry.iteration + 1, 0xf, 0x0);
      _renderer.depth_clear(*pair.first->portal_mesh,
                            next_stencil_ref, next_stencil_test_mask);
    }

    auto strict_stencil_test_mask = combine_mask(entry.iteration, 0xf, 0x0);
    auto translate = glm::translate(glm::mat4{}, _player.get_position());
    // Render objects in the source chunk.
    for (const auto& pair : portals_added) {
      if (pair.first->chunk_name == _active_chunk) {
        auto transform =
            entry.orientation * portal_matrix(*pair.first) *
            inv_orientation * translate;
        _renderer.world(transform);
        _renderer.draw(
            _player.get_mesh(), stencil_ref, strict_stencil_test_mask);
      }
    }

    // Render objects in the target chunk. Are these really both necessary?
    if (entry.iteration && entry.chunk == &it->second) {
      _renderer.world(entry.orientation * inv_orientation * translate);
      _renderer.draw(
          _player.get_mesh(), stencil_ref, strict_stencil_test_mask);
    }
  }
  _renderer.grain(1. / 32);
  _renderer.render();
}
