# Exports blender file to mobius world file.
import bpy
import math
import os

# Should be kept consistent with the definitions in mobius.proto.
PHYSICAL = 1
VISIBLE = 2

class proto(object):
  pass

def read_rgb(rgb):
  v = proto()
  v.r = rgb[0]
  v.g = rgb[1]
  v.b = rgb[2]
  return v

def read_vec3(vec3):
  # Blender axes are swapped compared to Mobius.
  v = proto()
  v.x = vec3[1]
  v.y = vec3[2]
  v.z = vec3[0]
  return v

def read_tri(tri):
  v = proto()
  v.a = tri[0]
  v.b = tri[1]
  v.c = tri[2]
  return v

def read_quad(quad):
  v = proto()
  v.a = quad[0]
  v.b = quad[1]
  v.c = quad[2]
  v.d = quad[3]
  return v

def make_mesh():
  v = proto()
  v.vertex = []
  v.geometry = []
  v.submesh = []
  return v

def has_property(obj, name):
  return name in obj.game.properties

def get_int_property(obj, name):
  assert(obj.game.properties[name].type == 'INT')
  return obj.game.properties[name].value

def get_bool_property(obj, name):
  assert(obj.game.properties[name].type == 'BOOL')
  return obj.game.properties[name].value

def get_float_property(obj, name):
  assert(obj.game.properties[name].type == 'FLOAT')
  return obj.game.properties[name].value

def get_string_property(obj, name):
  assert(obj.game.properties[name].type == 'STRING')
  return obj.game.properties[name].value

def export_submesh(mesh, obj, data):
  submesh = proto()
  geometry = proto()
  geometry.tri = []
  geometry.quad = []

  submesh.geometry = len(mesh.geometry)
  mesh.submesh.append(submesh)
  mesh.geometry.append(geometry)

  if has_property(obj, "flags"):
    submesh.flags = get_int_property(obj, "flags")
  else:
    submesh.flags = 0
  submesh.material = proto()
  submesh.material.colour = read_rgb([1, 1, 1])

  submesh.scale = read_vec3(obj.scale)
  submesh.translate = read_vec3(obj.location)

  start_index = len(mesh.vertex)
  for vertex in obj.data.vertices:
    mesh.vertex.append(read_vec3(vertex.co))

  obj.data.calc_tessface()
  for face in obj.data.tessfaces:
    if len(face.vertices) == 3:
      geometry.tri.append(read_tri(face.vertices))
    if len(face.vertices) == 4:
      geometry.quad.append(read_quad(face.vertices))

def portal_normal(data):
  v = read_vec3([0.0, 0.0, 0.0])
  for t in data.vertices:
    vt = read_vec3(t.co)
    v.x += vt.x
    v.y += vt.y
    v.z += vt.z
  v.x = -v.x / len(data.vertices)
  v.y = -v.y / len(data.vertices)
  v.z = -v.z / len(data.vertices)
  length = math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z)
  v.x /= length
  v.y /= length
  v.z /= length
  return v

def portal_up(obj):
  if not has_property(obj, "portal_up"):
    return read_vec3([0, 0, 1])
  up = get_string_property(obj, "portal_up")
  if up == "+X":
    return read_vec3([1, 0, 0])
  if up == "-X":
    return read_vec3([-1, 0, 0])
  if up == "+Y":
    return read_vec3([0, 1, 0])
  if up == "-Y":
    return read_vec3([0, -1, 0])
  if up == "+Z":
    return read_vec3([0, 0, 1])
  if up == "-Z":
    return read_vec3([0, 0, -1])
  return read_vec3([0, 0, 1])

def portal_orientation(obj):
  v = proto()
  v.origin = read_vec3(obj.location)
  v.normal = portal_normal(obj.data)
  v.up = portal_up(obj)
  return v

def export_portal(current_chunk_name, obj):
  portal = proto()
  portal.chunk_name = get_string_property(obj, "portal_target")

  portal.portal_mesh = make_mesh()
  export_submesh(portal.portal_mesh, obj, obj.data)
  portal.portal_mesh.submesh[0].flags = VISIBLE | PHYSICAL

  # Could these be calculated by the script from something simpler?
  if has_property(obj, "local_id"):
    portal.local_id = get_int_property(obj, "local_id")
  if has_property(obj, "remote_id"):
    portal.remote_id = get_int_property(obj, "remote_id")

  # Look up the remote portal to calculate orientation. If we want to allow
  # unusual portals (i.e. with no corresponding remote portal) we might need to
  # do something slightly different.
  remote_obj = None
  if portal.chunk_name in bpy.data.scenes:
    remote_scene = bpy.data.scenes[portal.chunk_name]
    for t in remote_scene.objects:
      if (has_property(t, "portal_target") and
          get_string_property(t, "portal_target") == current_chunk_name and
          has_property(t, "remote_id") and
          get_int_property(t, "remote_id") == portal.local_id):
        remote_obj = t
        break

  portal.local = portal_orientation(obj)
  if remote_obj:
    portal.remote = portal_orientation(remote_obj)
  return portal

def export_chunk(scene):
  chunk = proto()
  chunk.name = scene.name
  chunk.mesh = make_mesh()
  chunk.portal = []

  for obj in scene.objects:
    has_flags = has_property(obj, "flags") and get_int_property(obj, "flags")
    if obj.type == 'MESH' and has_flags:
      export_submesh(chunk.mesh, obj, obj.data)
    if obj.type == 'MESH' and has_property(obj, "portal_target"):
      chunk.portal.append(export_portal(scene.name, obj))
  return chunk

def write_proto(v, root=True, indent=0):
  if isinstance(v, str):
    return "\"%s\"" % v
  if isinstance(v, int):
    return str(v)
  if isinstance(v, float):
    return str(round(v, 2))

  indent_str = "  " * indent
  s = ""
  d = v.__dict__
  for key in d:
    value = d[key]
    if isinstance(value, list):
      for item in value:
        w = write_proto(item, False, 1 + indent)
        s += "%s%s: %s\n" % (indent_str, key, w)
    else:
      w = write_proto(value, False, 1 + indent)
      s += "%s%s: %s\n" % (indent_str, key, w)

  if root:
    return s
  indent_str = "  " * (indent - 1)
  return "{\n%s%s}" % (s, indent_str)

world = proto()
world.chunk = []
for scene in bpy.data.scenes:
  print("exporting chunk '%s'" % scene.name)
  world.chunk.append(export_chunk(scene))

output = write_proto(world)
with open(os.environ["EXPORT_PATH"], "w") as f:
  f.write(output)
