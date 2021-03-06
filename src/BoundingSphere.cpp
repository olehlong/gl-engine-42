//
// BoundingSphere.cpp for glEngine in /home/michar_l//gl-engine-42/src
// 
// Made by loick michard
// Login   <michar_l@epitech.net>
// 
// Started on  Wed May  2 16:41:28 2012 loick michard
// Last update Tue Jul 10 15:37:39 2012 loick michard
//

#include <BoundingSphere.hpp>
#include <Geometries.hpp>

gle::BoundingSphere::BoundingSphere() :
  _center(0, 0, 0), _absoluteRadius(0),
  _debugMaterial(NULL), _debugMesh(NULL)
{
}

gle::BoundingSphere::~BoundingSphere()
{
  delete _debugMaterial;
  delete _debugMesh;
}


gle::BoundingVolume* gle::BoundingSphere::duplicate() const
{
  gle::BoundingSphere* sphere = new gle::BoundingSphere();

  sphere->_debugMaterial = _debugMaterial;
  sphere->_center = _center;
  sphere->_absoluteCenter = _absoluteCenter;
  sphere->_radius = _radius;
  sphere->_absoluteRadius = _absoluteRadius;
  if (_debugMesh)
    sphere->_debugMesh = new Mesh(*_debugMesh);
  else
    sphere->_debugMesh = NULL;
  sphere->_min = _min;
  sphere->_max = _max;
  return (sphere);
}

void gle::BoundingSphere::setBestFit(const GLfloat* vertexes, GLsizeiptr size)
{
  GLsizeiptr nbVertexes = size / gle::Mesh::VertexAttributeSizeCoords;
  
   _center.x = 0;
  _center.y = 0;
  _center.z = 0;
  _radius = -1;
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    {
      _center.x += vertexes[i * 3] / nbVertexes;
      _center.y += vertexes[i * 3 + 1] / nbVertexes;
      _center.z += vertexes[i * 3 + 2] / nbVertexes;
    }
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    {
      GLfloat distance = (_center.x - vertexes[i * 3]) * (_center.x - vertexes[i * 3]) +
	(_center.y - vertexes[i * 3 + 1]) * (_center.y - vertexes[i * 3 + 1]) +
	(_center.z - vertexes[i * 3 + 2]) * (_center.z - vertexes[i * 3 + 2]);
      if (_radius == -1 || distance > _radius)
	_radius = distance;
    }
  _radius = sqrt(_radius);
  _min = _center;
  _min -= _radius;
  _max = _center;
  _max += _radius;
}

void gle::BoundingSphere::setBestFit(const GLfloat* datas, GLsizeiptr offset, GLsizeiptr attributeSize, GLsizeiptr nbVertexes)
{
  _center.x = 0;
  _center.y = 0;
  _center.z = 0;
  _radius = -1;
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    {
      _center.x += datas[offset + i * attributeSize] / nbVertexes;
      _center.y += datas[offset + i * attributeSize + 1] / nbVertexes;
      _center.z += datas[offset + i * attributeSize + 2] / nbVertexes;
    }
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    {
      GLfloat distance = (_center.x - datas[offset + i * attributeSize]) * (_center.x - datas[offset + i * attributeSize]) +
	(_center.y - datas[offset + i * attributeSize + 1]) * (_center.y - datas[offset + i * attributeSize + 1]) +
	(_center.z - datas[offset + i * attributeSize + 2]) * (_center.z - datas[offset + i * attributeSize + 2]);
      if (_radius == -1 || distance > _radius)
	_radius = distance;
    }
  _radius = sqrt(_radius);
  _min = _center;
  _min -= _radius;
  _max = _center;
  _max += _radius;
}

gle::Mesh* gle::BoundingSphere::getDebugMesh(gle::Mesh* mesh)
{
  if (!_debugMaterial)
    _debugMaterial->setAmbientColor(gle::Color<GLfloat>(1.0, 0, 0));
  if (!_debugMesh)
    {
      _debugMesh = gle::Geometries::Sphere(_debugMaterial, _radius, 15, 15, true); 
      _debugMesh->setPosition(_center);
      _debugMesh->setRasterizationMode(gle::Mesh::Line);
      
      Matrix4<GLfloat> mvMatrix = mesh->getTransformationMatrix();
      Matrix3<GLfloat> normalMatrix;

      mvMatrix.translate(_center);
      Matrix4<GLfloat> inverse(mvMatrix);
      inverse.inverse();
      normalMatrix = inverse;
      normalMatrix.transpose();
      _debugMesh->setMatrices(mvMatrix, normalMatrix);
      _debugMesh->setRasterizationMode(gle::Mesh::Line);
    }
  return (_debugMesh);
}

void	gle::BoundingSphere::update(const Matrix4<GLfloat>& mvMatrix)
{
  Vector3<GLfloat> point = _center;
  point.x += _radius;
  _absoluteCenter = _center;
  _absoluteCenter *= mvMatrix;
  point *= mvMatrix;
  _absoluteRadius = sqrt((point.x - _absoluteCenter.x) * (point.x - _absoluteCenter.x) +
		 (point.y - _absoluteCenter.y) * (point.y - _absoluteCenter.y) +
		 (point.z - _absoluteCenter.z) * (point.z - _absoluteCenter.z));
  if (_debugMesh)
    {
      Matrix4<GLfloat> moveMatrix = mvMatrix;
      Matrix3<GLfloat> normalMatrix;

      moveMatrix.translate(_center);
      Matrix4<GLfloat> inverse(moveMatrix);
      inverse.inverse();
      normalMatrix = inverse;
      normalMatrix.transpose();
      _debugMesh->setMatrices(moveMatrix, normalMatrix);
      _debugMesh->setRasterizationMode(gle::Mesh::Line);
    }
  _min = _absoluteCenter;
  _min -= _absoluteRadius;
  _max = _absoluteCenter;
  _max += _absoluteRadius;
}

const gle::Vector3<float>& gle::BoundingSphere::getMaxPoint()
{
  return (_max);
}

const gle::Vector3<float>& gle::BoundingSphere::getMinPoint()
{
  return (_min);
}

const gle::Vector3<float>& gle::BoundingSphere::getCenter()
{
  return (_absoluteCenter);
}

bool gle::BoundingSphere::isInFrustum(const GLfloat frustum[6][4]) const
{
  for(int i = 0; i < 6; i++)
    {
      if (frustum[i][0] * _min.x + frustum[i][1] * _min.y + frustum[i][2] * _min.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _max.x + frustum[i][1] * _min.y + frustum[i][2] * _min.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _min.x + frustum[i][1] * _max.y + frustum[i][2] * _min.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _max.x + frustum[i][1] * _max.y + frustum[i][2] * _min.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _min.x + frustum[i][1] * _min.y + frustum[i][2] * _max.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _max.x + frustum[i][1] * _min.y + frustum[i][2] * _max.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _min.x + frustum[i][1] * _max.y + frustum[i][2] * _max.z + frustum[i][3] > 0)
        continue;
      if (frustum[i][0] * _max.x + frustum[i][1] * _max.y + frustum[i][2] * _max.z + frustum[i][3] > 0)
        continue;
      return (false);
    }
  return (true);
}
