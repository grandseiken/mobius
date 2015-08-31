#include "world.h"
#include "mesh.h"
#include "proto_util.h"
#include "render.h"
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
  if (it != _chunks.end()) {
    _player.update(controls, *it->second.mesh);
  }
  _renderer.camera(
    _player.get_head_position(), _player.get_look_position(), {0, 1, 0});
  _renderer.light(_player.get_head_position(), 1.f);
}

void World::render() const
{
  _renderer.clear();
  auto it = _chunks.find(_active_chunk);
  if (it != _chunks.end()) {
    _renderer.world(glm::mat4{});
    _renderer.mesh(*it->second.mesh, 0);

    uint32_t stencil = 0;
    for (const auto& portal : it->second.portals) {
      auto jt = _chunks.find(portal.chunk_name);
      if (jt == _chunks.end()) {
        continue;
      }
      auto local = orientation_matrix(portal.local, false);
      auto remote = orientation_matrix(portal.remote, true);
      ++stencil;

      _renderer.world(glm::mat4{});
      _renderer.stencil(*portal.portal_mesh, stencil);

      _renderer.world(glm::inverse(local) * remote);
      _renderer.mesh(*jt->second.mesh, stencil);
    }
  }
  _renderer.grain(1. / 32);
  _renderer.render();
}
