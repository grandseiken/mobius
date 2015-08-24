vertex: {x: -1, y: -1, z: -1}
vertex: {x:  1, y: -1, z: -1}
vertex: {x: -1, y:  1, z: -1}
vertex: {x:  1, y:  1, z: -1}
vertex: {x: -1, y: -1, z:  1}
vertex: {x:  1, y: -1, z:  1}
vertex: {x: -1, y:  1, z:  1}
vertex: {x:  1, y:  1, z:  1}

faces: {
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
    colour: {r: .8, g: .29, b: .09}
  }
  faces: 0
  scale: {x: .125, y: .5, z: .125}
  translate: {x: 0, y: .5, z: 0}
}

submesh: {
  flags: 1
  faces: 0
  scale: {x: .0625, y: .5, z: .0625}
  translate: {x: 0, y: .5, z: 0}
}

submesh: {
  flags: 1
  faces: 0
  scale: {x: .125, y: .25, z: .0625}
  translate: {x: 0, y: .5, z: 0}
}

submesh: {
  flags: 1
  faces: 0
  scale: {x: .0625, y: .25, z: .125}
  translate: {x: 0, y: .5, z: 0}
}
