#ifndef MOBIUS_PROTO_UTIL_H
#define MOBIUS_PROTO_UTIL_H

#include "../gen/mobius.pb.h"
#include <glm/vec3.hpp>
#include <fstream>

template<typename T>
T load_proto(const std::string& path)
{
  T proto;
  std::ifstream ifstream{path};
  proto.ParseFromIstream(&ifstream);
  return proto;
}

inline
glm::vec3 load_vec3(const mobius::proto::vec3& v)
{
  return {v.x(), v.y(), v.z()};
}

inline
glm::vec3 load_rgb(const mobius::proto::rgb& v)
{
  return {v.r(), v.g(), v.b()};
}

#endif
