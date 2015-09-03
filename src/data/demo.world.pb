chunk: {
  name: "entryway"

  mesh: {
    vertex: {x: -1, y: -1, z: -1}
    vertex: {x:  1, y: -1, z: -1}
    vertex: {x: -1, y:  1, z: -1}
    vertex: {x:  1, y:  1, z: -1}
    vertex: {x: -1, y: -1, z:  1}
    vertex: {x:  1, y: -1, z:  1}
    vertex: {x: -1, y:  1, z:  1}
    vertex: {x:  1, y:  1, z:  1}

    vertex: {x: -.25, y:  -1, z: -1}
    vertex: {x:  .25, y:  -1, z: -1}
    vertex: {x: -.25, y: -.5, z: -1}
    vertex: {x:  .25, y: -.5, z: -1}
    vertex: {x: -.25, y: 1, z: -.25}
    vertex: {x:  .25, y: 1, z: -.25}
    vertex: {x: -.25, y: 1, z:  .25}
    vertex: {x:  .25, y: 1, z:  .25}

    vertex: {x: -.25, y:  -1, z: -1.25}
    vertex: {x:  .25, y:  -1, z: -1.25}
    vertex: {x: -.25, y: -.5, z: -1.25}
    vertex: {x:  .25, y: -.5, z: -1.25}
    vertex: {x: -.25, y: 1.25, z: -.25}
    vertex: {x:  .25, y: 1.25, z: -.25}
    vertex: {x: -.25, y: 1.25, z:  .25}
    vertex: {x:  .25, y: 1.25, z:  .25}

    geometry: {
      quad: {a: 0, b: 2, c: 3, d: 1}
      quad: {a: 4, b: 5, c: 7, d: 6}
      quad: {a: 0, b: 4, c: 6, d: 2}
      quad: {a: 1, b: 3, c: 7, d: 5}
      quad: {a: 0, b: 1, c: 5, d: 4}
      quad: {a: 2, b: 6, c: 7, d: 3}
    }

    geometry: {
      quad: {a: 4, b: 6, c: 7, d: 5}
      quad: {a: 0, b: 2, c: 6, d: 4}
      quad: {a: 1, b: 5, c: 7, d: 3}
      quad: {a: 0, b: 4, c: 5, d: 1}

      quad: {a: 0, b: 8, c: 10, d: 2}
      quad: {a: 10, b: 11, c: 3, d: 2}
      quad: {a: 11, b: 9, c: 1, d: 3}
      quad: {a: 8, b: 9, c: 17, d: 16}
      quad: {a: 8, b: 16, c: 18, d: 10}
      quad: {a: 18, b: 19, c: 11, d: 10}
      quad: {a: 9, b: 11, c: 19, d: 17}

      quad: {a: 2, b: 12, c: 14, d: 6}
      quad: {a: 14, b: 15, c: 7, d: 6}
      quad: {a: 15, b: 13, c: 3, d: 7}
      quad: {a: 13, b: 12, c: 2, d: 3}
      quad: {a: 12, b: 13, c: 21, d: 20}
      quad: {a: 12, b: 20, c: 22, d: 14}
      quad: {a: 22, b: 23, c: 15, d: 14}
      quad: {a: 13, b: 15, c: 23, d: 21}
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
      geometry: 1
      scale: {x: 4, y: 4, z: 4}
      translate: {x: 0, y: 2, z: 0}
    }
  }

  portal: {
    chunk_name: "entryway"
    local: {
      origin: {x: 0, y: -1, z: -4}
      normal: {x: 0, y: 0, z: 1}
      up: {x: 0, y: 1, z: 0}
    }
    remote: {
      origin: {x: 0, y: 7, z: 0}
      normal: {x: 0, y: -1, z: 0}
      up: {x: 0, y: 0, z: -1}
    }
    portal_mesh: {
      vertex: {x: -1, y: -1, z: 0}
      vertex: {x:  1, y: -1, z: 0}
      vertex: {x: -1, y:  1, z: 0}
      vertex: {x:  1, y:  1, z: 0}

      geometry: {
        quad: {a: 0, b: 1, c: 3, d: 2}
      }

      submesh: {
        flags: 3
        geometry: 0
        translate: {x: 0, y: -1, z: -5}
      }

      submesh: {
        flags: 2
        geometry: 0
        translate: {x: 0, y: -1, z: -4}
      }
    }
  }

  portal: {
    chunk_name: "entryway"
    local: {
      origin: {x: 0, y: 6, z: 0}
      normal: {x: 0, y: -1, z: 0}
      up: {x: 0, y: 0, z: -1}
    }
    remote: {
      origin: {x: 0, y: -1, z: -5}
      normal: {x: 0, y: 0, z: 1}
      up: {x: 0, y: 1, z: 0}
    }
    portal_mesh: {
      vertex: {x: -1, y: 0, z: -1}
      vertex: {x:  1, y: 0, z: -1}
      vertex: {x: -1, y: 0, z:  1}
      vertex: {x:  1, y: 0, z:  1}

      geometry: {
        quad: {a: 0, b: 1, c: 3, d: 2}
      }

      submesh: {
        flags: 3
        geometry: 0
        translate: {x: 0, y: 7, z: 0}
      }

      submesh: {
        flags: 2
        geometry: 0
        translate: {x: 0, y: 6, z: 0}
      }
    }
  }
}
