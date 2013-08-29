#include "octnode.h"
OctNode::OctNode()
{
    this->dataFlag = 0;
}

OctNode::~OctNode()
{
    if(this->dataFlag)
    {
        delete[] brickData;
    }
}

void OctNode::setBrick(float * buf)
{
    brickData = buf;
}

float * OctNode::getBrick()
{
    return brickData;
}

void OctNode::print()
{
    std::cout << "-----------------"<<  std::endl;
    std::cout << "Node at L"<< level << ": "<<  std::endl;
    std::cout << "Parent: "<< parent <<  std::endl;
    std::cout << "Child: "<< child <<  std::endl;
    std::cout << "Msd: "<< msdFlag <<  std::endl;
    std::cout << "Data: "<< dataFlag <<  std::endl;
    std::cout << "Brick id: ["<< brickId[0] << ", "<< brickId[1] << ", "<< brickId[2] << "]" << std::endl;
    std::cout << "Pool id: ["<< poolId[0] << ", "<< poolId[1] << ", "<< poolId[2] << "]" << std::endl;
}

void OctNode::calcPoolId(unsigned int poolPower, unsigned int brickNumber)
{
    // Bitwise operations used here:
    // X / 2^n = X >> n
    // X % 2^n = X & (2^n - 1)
    // 2^n = 1 << n
    
    poolId[0] = (brickNumber & ((1 << poolPower) - 1));
    poolId[1] = (brickNumber & ((1 << poolPower*2) - 1)) >> poolPower;
    poolId[2] = brickNumber >> (poolPower*2);
    
    //~ std::cout << "Calculated pool id from brick number " <<brickNumber<<": [" << poolId[0] << " "<< poolId[1] << " "<< poolId[2] << "]" << std::endl;
}


void OctNode::calcBrickId(unsigned int octant, OctNode * par)
{
    // Bitwise operations used here:
    // X / 2^n = X >> n
    // X % 2^n = X & (2^n - 1)
    // 2^n = 1 << n
    
    // The 3D index of the octant
    unsigned int oct_x = ((octant & 3) & 1);
    unsigned int oct_y = (octant & 3) >> 1;
    unsigned int oct_z = octant >> 2;
    
    // The parents brick id
    unsigned int * parentBrickId = par->getBrickId();
    
    // The brick id of this brick
    this->brickId[0] = parentBrickId[0]*2 + oct_x;
    this->brickId[1] = parentBrickId[1]*2 + oct_y;
    this->brickId[2] = parentBrickId[2]*2 + oct_z;
}

void OctNode::setDataFlag(unsigned int flag)
{
    this->dataFlag = flag;
}
void OctNode::setMsdFlag(unsigned int flag)
{
    this->msdFlag = flag;
}
void OctNode::setParent(unsigned int index)
{
    this->parent = index;
}
void OctNode::setChild(unsigned int index)
{
    this->child = index;
}
void OctNode::setLevel(unsigned int index)
{
    this->level = index;
}
void OctNode::setBrickId(unsigned int x, unsigned int y, unsigned int z)
{
    this->brickId[0] = x;
    this->brickId[1] = y;
    this->brickId[2] = z;
}
void OctNode::setPoolId(unsigned int x, unsigned int y, unsigned int z)
{
    this->poolId[0] = x;
    this->poolId[1] = y;
    this->poolId[2] = z;
}

unsigned int OctNode::getDataFlag()
{
    return this->dataFlag;
}
unsigned int OctNode::getMsdFlag()
{
    return this->msdFlag;
}

unsigned int OctNode::getParent()
{
    return this->parent;
}
unsigned int OctNode::getChild()
{
    return this->child;
}

unsigned int OctNode::getLevel()
{
    return this->level;
}
unsigned int * OctNode::getBrickId()
{
    return this->brickId;
}
unsigned int OctNode::getBrickId1D()
{
    // Requires the correct level to be set first
    
    unsigned int brickId1D = brickId[0] + brickId[1]*(1 << level) + brickId[2]*(1 << level)*(1 << level);
    
    return brickId1D;
}
unsigned int * OctNode::getPoolId()
{
    return this->poolId;
}
