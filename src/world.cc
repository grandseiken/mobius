#include "world.h"
#include "mesh.h"
#include "proto_util.h"
#include "render.h"
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
}

World::World(const std::string& path, Renderer& renderer)
: _renderer(renderer)
, _player{_collision, {0, 3, 0}}
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
    environment.push_back(
        {jt->second.mesh.get(), portal_matrix(portal) * _orientation});
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
    _orientation = portal_matrix(portal) * _orientation;
    break;
  }
}

void World::render() const
{
  auto it = _chunks.find(_active_chunk);
  if (it == _chunks.end()) {
    return;
  }

  _renderer.clear();
  auto head = _player.get_head_position();
  auto look = _player.get_look_position();
  _renderer.camera(head, look, {0, 1, 0});
  _renderer.light(head, 1.f);

  _renderer.world(_orientation);
  _renderer.depth(*it->second.mesh, 0);
  _renderer.draw(*it->second.mesh, 0);

  uint32_t stencil = 0;
  for (const auto& portal : it->second.portals) {
    auto jt = _chunks.find(portal.chunk_name);
    if (jt == _chunks.end()) {
      continue;
    }

    // Extremely simple visibility determination.
    bool visible = false;
    for (const auto& v : portal.portal_mesh->physical_vertices()) {
      glm::vec3 vt{_orientation * glm::vec4{v, 1}};
      if (glm::dot(look - head, vt - head) >= 0) {
        visible = true;
        break;
      }
    }
    if (!visible) {
      continue;
    }
    ++stencil;

    // TODO: something is very slow when there are large stencils covering most
    // of the screen (it seems).
    _renderer.world(_orientation);
    // To avoid awkwardly-placed portals being seen through other stencils and
    // messing up the buffer, we have to render first with depth-writing enabled
    // and then manually clear the depth. (A possibly-faster approach would be
    // to divide up the depth buffer and use different ranges for each level of
    // the portal tree.)
    _renderer.stencil(*portal.portal_mesh, stencil);
    _renderer.depth_clear(*portal.portal_mesh, stencil);

    // We have to clip behind the portal so that we don't see overlapping
    // geometry hanging about.
    auto normal_orientation =
        glm::transpose(glm::inverse(glm::mat3{_orientation}));
    glm::vec3 clip_point{_orientation * glm::vec4{portal.local.origin, 1}};
    auto clip_normal = -normal_orientation * portal.local.normal;
    auto matrix = portal_matrix(portal);
    _renderer.world(matrix * _orientation, clip_point, clip_normal);
    _renderer.depth(*jt->second.mesh, stencil);
    _renderer.draw(*jt->second.mesh, stencil);

    if (jt->first == _active_chunk) {
      auto translate = glm::translate(glm::mat4{}, _player.get_position());
      _renderer.world(matrix * translate);
      _renderer.draw(_player.get_mesh(), stencil);
    }
  }
  _renderer.grain(1. / 32);
  _renderer.render();
}
