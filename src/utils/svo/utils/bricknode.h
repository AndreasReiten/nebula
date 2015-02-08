#ifndef OCTNODE_H
#define OCTNODE_H

/*
 * This class represents a node in the sparse voxel octtree. The nodes lie in an array. This is a convenience class to make the data structure more managable. 
 * */

class BrickNode {
    public:
        BrickNode();
        ~BrickNode();

        void setDataFlag(unsigned int flag);
        void setMsdFlag(unsigned int flag);

        void setParent(unsigned int index);
        void setChild(unsigned int index);

        void setLevel(unsigned int index);
        void setBrickId(unsigned int x, unsigned int y, unsigned int z);
        void setPoolId(unsigned int x, unsigned int y, unsigned int z);

        unsigned int getDataFlag();
        unsigned int getMsdFlag();

        unsigned int getParent();
        unsigned int getChild();

        unsigned int getLevel();
        unsigned int * getBrickId();
        unsigned int getBrickId1D();
        unsigned int * getPoolId();
        void calcBrickId(unsigned int octant, BrickNode * par);
        void calcPoolId(unsigned int poolPower, unsigned int brickNumber);

        void print();

    private:
        unsigned int dataFlag;
        unsigned int msdFlag;

        unsigned int parent;
        unsigned int child;

        unsigned int level;
        unsigned int brick_id[3];
        unsigned int poolId[3];
};
#endif
