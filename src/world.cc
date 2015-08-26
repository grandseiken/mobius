#include "world.h"
#include "proto_util.h"

World::World(const std::string& path)
{
  auto world = load_proto<mobius::proto::world>(path);
  for (const auto& chunk_proto : world.chunk()) {
    Chunk& chunk = _chunks[chunk_proto.name()];
    chunk.mesh = Mesh{chunk_proto.mesh()};
    for (const auto& portal : chunk_proto.portal()) {
      chunk.portals.push_back({
          portal.chunk_name(),
          Mesh(portal.portal_mesh()),
          load_vec3(portal.local_origin()),
          load_vec3(portal.local_normal()),
          load_vec3(portal.remote_origin()),
          load_vec3(portal.remote_normal())});
    }
  }
}
