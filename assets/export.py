# Exports blender file to mobius world file.
import bpy
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
  # Blender Y and Z axes are swapped compared to Mobius.
  v = proto()
  v.x = vec3[0]
  v.y = vec3[2]
  v.z = vec3[1]
  return v

def read_tri(tri):
  # Blender winding direction is the opposite of Mobius.
  v = proto()
  v.a = tri[2]
  v.b = tri[1]
  v.c = tri[0]
  return v

def read_quad(quad):
  # Blender winding direction is the opposite of Mobius.
  v = proto()
  v.a = quad[3]
  v.b = quad[2]
  v.c = quad[1]
  v.d = quad[0]
  return v

def export_submesh(mesh, obj, data):
  submesh = proto()
  geometry = proto()
  geometry.tri = []
  geometry.quad = []

  submesh.geometry = len(mesh.geometry)
  mesh.submesh.append(submesh)
  mesh.geometry.append(geometry)

  submesh.flags = PHYSICAL | VISIBLE
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

def export_chunk(scene):
  chunk = proto()
  chunk.name = scene.name
  chunk.mesh = proto()
  chunk.mesh.vertex = []
  chunk.mesh.geometry = []
  chunk.mesh.submesh = []

  for obj in scene.objects:
    if obj.type == 'MESH':
      export_submesh(chunk.mesh, obj, obj.data)
  return chunk

def write_proto(v, root=True, indent=0):
  if isinstance(v, str):
    return "\"%s\"" % v
  if isinstance(v, int) or isinstance(v, float):
    return str(v)

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
