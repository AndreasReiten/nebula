#include "bricknode.h"

// Bitwise operations used here:
// X / 2^n = X >> n
// X % 2^n = X & (2^n - 1)
// 2^n = 1 << n


BrickNode::BrickNode()
{
    this->dataFlag = 0;
}

BrickNode::~BrickNode()
{
}

void BrickNode::print()
{
    std::cout << "-----------------"<<  std::endl;
    std::cout << "Node at L"<< level << ": "<<  std::endl;
    std::cout << "Parent: "<< parent <<  std::endl;
    std::cout << "Child: "<< child <<  std::endl;
    std::cout << "Msd: "<< msdFlag <<  std::endl;
    std::cout << "Data: "<< dataFlag <<  std::endl;
    std::cout << "Brick id: ["<< brick_id[0] << ", "<< brick_id[1] << ", "<< brick_id[2] << "]" << std::endl;
    std::cout << "Pool id: ["<< poolId[0] << ", "<< poolId[1] << ", "<< poolId[2] << "]" << std::endl;
}

void BrickNode::calcPoolId(unsigned int poolPower, unsigned int brickNumber)
{
    poolId[0] = (brickNumber & ((1 << poolPower) - 1));
    poolId[1] = (brickNumber & ((1 << poolPower*2) - 1)) >> poolPower;
    poolId[2] = brickNumber >> (poolPower*2);
}


void BrickNode::calcBrickId(unsigned int octant, BrickNode * par)
{


    if (this != par)
    {
        // The 3D index of the octant
        unsigned int oct_x = ((octant & 3) & 1);
        unsigned int oct_y = (octant & 3) >> 1;
        unsigned int oct_z = octant >> 2;

        // The parents brick id
        unsigned int * parentBrickId = par->getBrickId();

        // The brick id of this brick
        this->brick_id[0] = parentBrickId[0]*2 + oct_x;
        this->brick_id[1] = parentBrickId[1]*2 + oct_y;
        this->brick_id[2] = parentBrickId[2]*2 + oct_z;
    }
    else
    {
        this->brick_id[0] = 0;
        this->brick_id[1] = 0;
        this->brick_id[2] = 0;
    }
}

void BrickNode::setDataFlag(unsigned int flag)
{
    // A false here indicates an empty brick
    this->dataFlag = flag;
}
void BrickNode::setMsdFlag(unsigned int flag)
{
    this->msdFlag = flag;
}
void BrickNode::setParent(unsigned int index)
{
    this->parent = index;
}
void BrickNode::setChild(unsigned int index)
{
    this->child = index;
}
void BrickNode::setLevel(unsigned int index)
{
    this->level = index;
}
void BrickNode::setBrickId(unsigned int x, unsigned int y, unsigned int z)
{
    this->brick_id[0] = x;
    this->brick_id[1] = y;
    this->brick_id[2] = z;
}
void BrickNode::setPoolId(unsigned int x, unsigned int y, unsigned int z)
{
    this->poolId[0] = x;
    this->poolId[1] = y;
    this->poolId[2] = z;
}

unsigned int BrickNode::getDataFlag()
{
    return this->dataFlag;
}
unsigned int BrickNode::getMsdFlag()
{
    return this->msdFlag;
}

unsigned int BrickNode::getParent()
{
    return this->parent;
}
unsigned int BrickNode::getChild()
{
    return this->child;
}

unsigned int BrickNode::getLevel()
{
    return this->level;
}
unsigned int * BrickNode::getBrickId()
{
    return this->brick_id;
}
unsigned int BrickNode::getBrickId1D()
{
    // Requires the correct level to be set first

    unsigned int brickId1D = brick_id[0] + brick_id[1]*(1 << level) + brick_id[2]*(1 << level)*(1 << level);

    return brickId1D;
}
unsigned int * BrickNode::getPoolId()
{
    return this->poolId;
}
