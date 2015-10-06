#ifndef SMALLSTUFF_H
#define SMALLSTUFF_H

struct xyzw32
{
    float x, y, z, w;
};

struct xyzw64
{
    double x, y, z, w;
};

struct nodeinfo
{
    int level;
    int id_x, id_y, id_z;
    int side;
    double id_nxt_x, id_nxt_y, id_nxt_z;
    int n;
};

#endif // SMALLSTUFF_H

