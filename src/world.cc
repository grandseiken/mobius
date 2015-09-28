#include "world.h"
#include "mesh.h"
#include "proto_util.h"
#include "render.h"
#include "visibility.h"
#include <glm/vec4.hpp>
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

  static const uint32_t VALUE_BITS = 0x7f;
  static const uint32_t FLAG_BITS = 0x80;
  uint32_t combine_mask(bool flag, uint32_t value)
  {
    return (flag ? FLAG_BITS : 0) | (value & VALUE_BITS);
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
      portal.portal_mesh.reset(new Mesh{portal_proto.portal_mesh()});

      portal.local.origin = load_vec3(portal_proto.local().origin());
      portal.local.normal = load_vec3(portal_proto.local().normal());
      portal.local.up = load_vec3(portal_proto.local().up());

      portal.remote.origin = load_vec3(portal_proto.remote().origin());
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

void World::render(RenderMetrics& metrics) const
{
  auto it = _chunks.find(_active_chunk);
  if (it == _chunks.end()) {
    return;
  }

  const auto& head = _player.get_head_position();
  auto look_at = head + _player.get_look_direction();
  _renderer.perspective(
      _player.get_fov(), _player.get_z_near(), _player.get_z_far());
  _renderer.camera(head, look_at, {0, 1, 0});
  _renderer.light(head, 128.f);
  _renderer.clear();

  metrics.chunks = 0;
  metrics.depth = 1;
  metrics.breadth = 1;

  std::vector<chunk_entry> buffer_a;
  std::vector<chunk_entry> buffer_b;
  buffer_a.push_back({&it->second, nullptr, 0, {_orientation, {}}, {{}, {}}});

  // TODO: could rewrite to build the scene graph in one step, and render it in
  // another.
  for (uint32_t i = 0; i < MAX_ITERATIONS; ++i) {
    render_iteration(i, metrics, i % 2 ? buffer_b : buffer_a,
                                 i % 2 ? buffer_a : buffer_b);
  }
}

void World::render_iteration(
    uint32_t iteration, RenderMetrics& metrics,
    const std::vector<chunk_entry>& read_buffer,
    std::vector<chunk_entry>& write_buffer) const
{
  write_buffer.clear();
  if (read_buffer.empty()) {
    return;
  }
  metrics.depth = std::max(metrics.depth, 1 + iteration);

  bool last_iteration = iteration + 1 >= MAX_ITERATIONS;
  uint32_t iteration_stencil = 0;

  // For further iterations, these clip planes are mostly redundant - we could
  // perhaps just use this for the first iteration and only keep the near/far
  // depth planes after that.
  const auto view_clip_planes =
      calculate_view_frustum(_player, _renderer.get_aspect_ratio());

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
  for (const auto& entry : read_buffer) {
    auto visibility_clip_planes = view_clip_planes;
    for (const auto& plane : entry.data.clip_planes) {
      visibility_clip_planes.push_back(plane);
    }

    ++metrics.chunks;
    // Establish depth buffer for this chunk.
    uint32_t stencil_ref = combine_mask(false, entry.stencil);
    _renderer.world(entry.data.orientation, entry.data.clip_planes);
    _renderer.depth(*entry.chunk->mesh, stencil_ref, VALUE_BITS);

    render_objects_in_chunk(iteration, entry.chunk, entry.data, stencil_ref);
    // Renderer the chunk.
    _renderer.world(entry.data.orientation, entry.data.clip_planes);
    _renderer.draw(*entry.chunk->mesh, stencil_ref, VALUE_BITS);

    for (const auto& portal : entry.chunk->portals) {
      auto jt = _chunks.find(portal.chunk_name);
      bool is_source = entry.source &&
          portal.portal_id == entry.source->portal_id &&
          &portal != entry.source;

      const auto& head = _player.get_head_position();
      if (last_iteration || jt == _chunks.end() || is_source ||
          !mesh_visible(visibility_clip_planes, head,
                        entry.data.orientation, *portal.portal_mesh)) {
        continue;
      }
      // TODO: this should really warn when we reuse stencil bits.
      auto stencil = 1 + iteration_stencil++ % (VALUE_BITS - 1);
      metrics.breadth = std::max(metrics.breadth, iteration_stencil);

      auto orientation = entry.data.orientation * portal_matrix(portal);
      auto portal_frustum = calculate_bounding_frustum(
          _player, _renderer.get_aspect_ratio(),
          entry.data.orientation, portal);
      write_buffer.push_back({&jt->second, &portal, stencil,
                              {orientation, portal_frustum}, entry.data});

      auto portal_stencil_ref = combine_mask(true, entry.stencil);
      _renderer.stencil(
          *portal.portal_mesh, portal_stencil_ref,
          /* read */ VALUE_BITS, /* write */ FLAG_BITS, /* depth_eq */ false);
    }
  }

  _renderer.clear_stencil(VALUE_BITS);
  // This part could theoretically cause artifacts when portals visible
  // through different portals intersect exactly in camera space. However, if
  // the portal clipping planes are calculated perfectly (which isn't always
  // possible for complex portal meshes, since there is a hardware limitation
  // on the number of clipping planes), such intersections are impossible.
  // If such artifacts are encountered, we should improve the
  // calculate_bounding_frustum algorithm.
  for (const auto& entry : write_buffer) {
    auto portal_stencil_ref = combine_mask(true, entry.stencil);
    _renderer.world(entry.source_data.orientation,
                    entry.source_data.clip_planes);
    _renderer.stencil(
        *entry.source->portal_mesh, portal_stencil_ref,
        /* read */ FLAG_BITS, /* write */ VALUE_BITS, /* depth_eq */ true);
  }

  _renderer.clear_depth();
  _renderer.clear_stencil(FLAG_BITS);
}

void World::render_objects_in_chunk(
    uint32_t iteration, const Chunk* chunk,
    const world_data& data, uint32_t stencil_ref) const
{
  // Since we currently only have a player, "get objects in chunk" is just
  // "get the player in the active_chunk on nonzero iterations".
  // TODO: isn't right any more, but drawing the objects from the next
  // iteration in this stencil could result in overlapping spaces. Need to
  // think it through.
  auto it = _chunks.find(_active_chunk);
  if (it != _chunks.end() && chunk == &it->second && iteration) {
    auto inv_orientation = glm::inverse(_orientation);
    auto translate = glm::translate(glm::mat4{}, _player.get_position());

    _renderer.world(
        data.orientation * inv_orientation * translate, data.clip_planes);
    _renderer.draw(_player.get_mesh(), stencil_ref, VALUE_BITS);
  }
}
