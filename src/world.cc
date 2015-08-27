#include "world.h"
#include "mesh.h"
#include "proto_util.h"
#include "render.h"

World::World(const std::string& path, Renderer& renderer)
: _renderer(renderer)
, _player{_collision, {0, 8, 0}}
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
      portal.local_origin = load_vec3(portal_proto.local_origin());
      portal.local_normal = load_vec3(portal_proto.local_normal()),
      portal.remote_origin = load_vec3(portal_proto.remote_origin()),
      portal.remote_normal = load_vec3(portal_proto.remote_normal());
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
  _renderer.world(glm::mat4{});
  auto it = _chunks.find(_active_chunk);
  if (it != _chunks.end()) {
    _renderer.mesh(*it->second.mesh);
  }
  _renderer.grain(1. / 32);
  _renderer.render();
}
