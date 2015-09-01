vertex: {x: -1, y: -1, z: -1}
vertex: {x:  1, y: -1, z: -1}
vertex: {x: -1, y:  1, z: -1}
vertex: {x:  1, y:  1, z: -1}
vertex: {x: -1, y: -1, z:  1}
vertex: {x:  1, y: -1, z:  1}
vertex: {x: -1, y:  1, z:  1}
vertex: {x:  1, y:  1, z:  1}

geometry: {
  quad: {a: 0, b: 2, c: 3, d: 1}
  quad: {a: 4, b: 5, c: 7, d: 6}
  quad: {a: 0, b: 4, c: 6, d: 2}
  quad: {a: 1, b: 3, c: 7, d: 5}
  quad: {a: 0, b: 1, c: 5, d: 4}
  quad: {a: 2, b: 6, c: 7, d: 3}
}

geometry: {
  point: 0
  point: 1
  point: 2
  point: 3
  point: 4
  point: 5
  point: 6
  point: 7
}

submesh: {
  flags: 3
  material: {
    colour: {r: 1, g: 1, b: 1}
  }
  geometry: 0
  scale: {x: .125, y: .5, z: .125}
}

submesh: {
  flags: 1
  geometry: 1
  scale: {x: .0625, y: .5, z: .0625}
}

submesh: {
  flags: 1
  geometry: 1
  scale: {x: .125, y: .25, z: .0625}
}

submesh: {
  flags: 1
  geometry: 1
  scale: {x: .0625, y: .25, z: .125}
}
