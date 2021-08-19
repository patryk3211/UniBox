void update(inout ArrayVertex vertex) {
    uint x = uint(vertex.position[0]);
    uint y = uint(vertex.position[1]);
    uint z = uint(vertex.position[2]);
    if(checkBounds(x, y-1, z)) {
        if(isEmpty(x, y-1, z)) {
            vertex.position[1] -= 1;
            return;
        } else if(checkBounds(x-1, y-1, z) && isEmpty(x-1, y-1, z)) {
            vertex.position[1] -= 1;
            vertex.position[0] -= 1;
            return;
        } else if(checkBounds(x+1, y-1, z) && isEmpty(x+1, y-1, z)) {
            vertex.position[1] -= 1;
            vertex.position[0] += 1;
            return;
        }
    }
}