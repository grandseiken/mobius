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

submesh: {
  flags: 3
  material: {
    colour: {r: .16, g: .63, b: .60}
  }
  geometry: 0
  scale: {x: 2, y: 2, z: 2}
}

submesh: {
  flags: 3
  material: {
    colour: {r: .35, g: .43, b: .46}
  }
  geometry: 0
  scale: {x: 8, y: -8, z: 8}
  translate: {x: 0, y: 6, z: 0}
}
