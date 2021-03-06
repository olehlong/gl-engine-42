//
// Mesh.cpp for gle in /home/michar_l//glEngine
// 
// Made by loick michard
// Login   <michar_l@epitech.net>
// 
// Started on  Mon Feb 20 18:25:23 2012 loick michard
// Last update Tue Jul 10 15:36:17 2012 loick michard
//

#include <Mesh.hpp>
#include <BoundingBox.hpp>
#include <Renderer.hpp>
#include <Skeleton.hpp>

std::list<gle::Scene::MeshGroup> gle::Mesh::factorizeForDrawing(std::list<gle::Mesh*> meshes,
								bool ignoreBufferId,
								bool ignoreMaterial)
{  
  std::list<gle::Scene::MeshGroup> groups;

  while (meshes.size() > 0)
    {
      gle::Mesh* currentMesh = meshes.front();
      meshes.pop_front();
      Scene::MeshGroup currentGroup = {
	.meshes = {currentMesh},
	.uniformBufferId = currentMesh->getUniformBufferId(),
	.materialBufferId = currentMesh->getMaterialBufferId(),
	.rasterizationMode = static_cast<GLint>(currentMesh->getRasterizationMode()),
	.colorMap = (currentMesh->getMaterial() && currentMesh->getMaterial()->isColorMapEnabled())
	? currentMesh->getMaterial()->getColorMap() : NULL,
	.normalMap = (currentMesh->getMaterial() && currentMesh->getMaterial()->isNormalMapEnabled())
	? currentMesh->getMaterial()->getNormalMap() : NULL,
	.envMap = (currentMesh->getMaterial() && currentMesh->getMaterial()->isEnvMapEnabled())
	? currentMesh->getMaterial()->getEnvMap() : NULL
      };
      for (auto it = meshes.begin(); it != meshes.end();)
	{
	  if ((*it) == currentMesh || (*it)->canBeRenderedWith(currentGroup, ignoreBufferId, ignoreMaterial))
	    {
	      gle::Material* material = (*it)->getMaterial();
	      if (material)
	      	{
	      	  if (material->isColorMapEnabled())
	      	    currentGroup.colorMap = material->getColorMap();
	      	  if (material->isNormalMapEnabled())
	      	    currentGroup.normalMap = material->getNormalMap();
	      	  if (material->isEnvMapEnabled())
	      	    currentGroup.envMap = material->getEnvMap();
	      	}
	      currentGroup.meshes.push_front(*it);
	      it = meshes.erase(it);
	    }
	  else
	    ++it;
	}
      groups.push_front(currentGroup);
    }
  return (groups);
}

gle::Mesh::Mesh(Material* material, bool isDynamic)
  : gle::Scene::Node((isDynamic) ? gle::Scene::Node::DynamicMesh : gle::Scene::Node::StaticMesh),
    _primitiveType(Triangles),
    _rasterizationMode(Fill),
    _pointSize(1.0),
    _material(material),
    _indexes(NULL),
    _attributes(NULL),
    _nbIndexes(0),
    _nbVertexes(0),
    _boundingVolume(NULL),
    _uniformBufferId(-1),
    _materialBufferId(-1),
    _absoluteIndexes(false),
    _needUniformsUpdate(true),
    _uniforms(NULL), _skeleton(NULL), _skeletonId(-1),
    _needSetIdentifiers(true)
{
  _isDynamic = isDynamic;
  _indexes = new gle::Buffer<GLuint>(gle::Buffer<GLuint>::ElementArray,
				     gle::Buffer<GLuint>::StaticDraw);
}

gle::Mesh::Mesh(gle::Mesh const & other)
  : gle::Scene::Node(other),
    _primitiveType(other._primitiveType),
    _rasterizationMode(other._rasterizationMode),
    _pointSize(other._pointSize),
    _material(other._material),
    _indexes(NULL),
    _attributes(NULL),
    _nbIndexes(other._nbIndexes),
    _nbVertexes(other._nbVertexes),
    _boundingVolume(NULL),
    _uniformBufferId(-1),
    _materialBufferId(-1),
    _absoluteIndexes(other._absoluteIndexes),
    _needUniformsUpdate(true),
    _uniforms(NULL), _skeleton(other._skeleton), _skeletonId(other._skeletonId),
    _needSetIdentifiers(true)
{
  if (other._boundingVolume)
    _boundingVolume = other._boundingVolume->duplicate();
  else
    _boundingVolume = NULL;
  static int max = 0, nb = 0;
  max += _nbVertexes;
  nb++;
  if (other._indexes)
    _indexes =  new gle::Buffer<GLuint>(*other._indexes);
  if (other._attributes)
    _attributes = gle::MeshBufferManager::getInstance().duplicate(other._attributes);
  if (other._indexes && other._attributes && !_isDynamic)
    {
      GLuint* indexes = _indexes->map();
      GLuint oldOffset = other._attributes->getOffset() / VertexAttributesSize;
      GLuint newOffset = _attributes->getOffset() / VertexAttributesSize;
      for (GLuint i = 0; i < _indexes->getSize(); ++i)
  	indexes[i] = indexes[i] - oldOffset + newOffset;
      _indexes->unmap();
    }
}

gle::Mesh::~Mesh()
{
  if (_indexes)
    delete _indexes;
  if (_attributes)
    _attributes->release();
  if (_boundingVolume)
    delete _boundingVolume;
  if (_uniforms)
    delete [] _uniforms;
}

void gle::Mesh::setPrimitiveType(PrimitiveType type)
{
  _primitiveType = type;
}

gle::Mesh::PrimitiveType gle::Mesh::getPrimitiveType() const
{
  return (_primitiveType);
}

void gle::Mesh::setRasterizationMode(RasterizationMode rasterizationMode)
{
  _rasterizationMode = rasterizationMode;
}

gle::Mesh::RasterizationMode gle::Mesh::getRasterizationMode() const
{
  return (_rasterizationMode);
}

void gle::Mesh::setPointSize(GLfloat v)
{
  _pointSize = v;
}

GLfloat gle::Mesh::getPointSize() const
{
  return (_pointSize);
}

void gle::Mesh::setVertexAttributes(const GLfloat* attributes, GLsizeiptr nbVertexes)
{
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(attributes, nbVertexes * VertexAttributesSize);
  else
    _attributes = MeshBufferManager::getInstance()
      .resize(_attributes, nbVertexes * VertexAttributesSize, attributes);
  _nbVertexes = nbVertexes;
  createBoundingVolume(attributes, 0, VertexAttributesSize, nbVertexes);
  _needSetIdentifiers = true;
}

void gle::Mesh::setVertexes(const GLfloat* vertexes, GLsizeiptr size, bool boundingVolume)
{
  GLsizeiptr nbVertexes = size / VertexAttributeSizeCoords;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeCoords; ++j)
      attributes[i * VertexAttributesSize + j] =
	vertexes[i * VertexAttributeSizeCoords + j];
  attributes[0] = vertexes[0];
  _attributes->unmap();
  if (boundingVolume)
    createBoundingVolume(vertexes, 0, VertexAttributeSizeCoords, nbVertexes);
  _needSetIdentifiers = true;
}

void gle::Mesh::setNormals(const GLfloat* normals, GLsizeiptr size)
{
  GLsizeiptr nbVertexes = size / VertexAttributeSizeNormal;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeNormal; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords + j] =
	normals[i * VertexAttributeSizeNormal + j];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setTangents(const GLfloat* tangents, GLsizeiptr size)
{
  GLsizeiptr nbVertexes = size / VertexAttributeSizeTangent;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeTangent; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords + VertexAttributeSizeNormal + j] =
	tangents[i * VertexAttributeSizeTangent + j];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setTextureCoords(const GLfloat* textureCoords, GLsizeiptr size)
{
  GLsizeiptr nbVertexes = size / VertexAttributeSizeTextureCoords;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeTextureCoords; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords +
		 VertexAttributeSizeNormal + VertexAttributeSizeTangent + j] =
	textureCoords[i * VertexAttributeSizeTextureCoords + j];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setBones(const GLfloat* bones, GLsizeiptr size)
{
  GLsizeiptr nbVertexes = size / VertexAttributeSizeBone;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeBone; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords + VertexAttributeSizeNormal + 
		 VertexAttributeSizeTangent + VertexAttributeSizeTextureCoords + j] =
	bones[i * VertexAttributeSizeBone + j];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setIndexes(const GLuint* indexes, GLsizeiptr size)
{
  _nbIndexes = size;
  _indexes->resize(size, indexes);
  if (!_isDynamic)
    makeAbsoluteIndexes();
  _needSetIdentifiers = true;
}

void gle::Mesh::setVertexes(gle::Array<GLfloat> const &vertexes, bool boundingVolume)
{
  GLsizeiptr nbVertexes = vertexes.size() / VertexAttributeSizeCoords;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLsizeiptr j = 0; j < VertexAttributeSizeCoords; ++j)
      attributes[i * VertexAttributesSize + j] =
  	vertexes[(GLuint)(i * VertexAttributeSizeCoords + j)];
  _attributes->unmap();
  if (boundingVolume)
    createBoundingVolume((const GLfloat*)vertexes, 0, VertexAttributeSizeCoords, nbVertexes);
  _needSetIdentifiers = true;
}

void gle::Mesh::setNormals(gle::Array<GLfloat> const &normals)
{
  GLsizeiptr nbVertexes = normals.size() / VertexAttributeSizeNormal;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeNormal; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords + j] =
	normals[(GLuint)(i * VertexAttributeSizeNormal + j)];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setTangents(gle::Array<GLfloat> const &tangents)
{
  GLsizeiptr nbVertexes = tangents.size() / VertexAttributeSizeTangent;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeTangent; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords + VertexAttributeSizeNormal + j] =
	tangents[(GLuint)(i * VertexAttributeSizeTangent + j)];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setTextureCoords(gle::Array<GLfloat> const &textureCoords)
{
  GLsizeiptr nbVertexes = textureCoords.size() / VertexAttributeSizeTextureCoords;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeTextureCoords; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords +
		 VertexAttributeSizeNormal + VertexAttributeSizeTangent + j] =
	textureCoords[(GLuint)(i * VertexAttributeSizeTextureCoords + j)];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setBones(gle::Array<GLfloat> const &bones)
{
  GLsizeiptr nbVertexes = bones.size() / VertexAttributeSizeBone;

  _nbVertexes = nbVertexes;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();
  for (GLsizeiptr i = 0; i < nbVertexes; ++i)
    for (GLuint j = 0; j < VertexAttributeSizeTextureCoords; ++j)
      attributes[i * VertexAttributesSize + VertexAttributeSizeCoords +
		 VertexAttributeSizeNormal + VertexAttributeSizeTangent + 
		 VertexAttributeSizeBone + j] =
	bones[(GLuint)(i * VertexAttributeSizeBone + j)];
  _attributes->unmap();
  _needSetIdentifiers = true;
}

void gle::Mesh::setIndexes(gle::Array<GLuint> const &indexes)
{
  _nbIndexes = indexes.size();
  _indexes->resize(indexes.size(), (GLuint const *)indexes);
  if (!_isDynamic)
    makeAbsoluteIndexes();
  _needSetIdentifiers = true;
}

void gle::Mesh::setIdentifiers(GLuint meshId, GLuint materialId)
{
  if (_nbVertexes < 1)
    return ;
  if (!_attributes)
    _attributes = MeshBufferManager::getInstance()
      .store(NULL, _nbVertexes * VertexAttributesSize);
  GLfloat* attributes = _attributes->map();

  for (GLsizeiptr i = 0; i < _nbVertexes; ++i)
    {
      attributes[i * VertexAttributesSize
		 + VertexAttributeSizeCoords
		 + VertexAttributeSizeNormal
		 + VertexAttributeSizeTangent
		 + VertexAttributeSizeTextureCoords
		 + VertexAttributeSizeBone
		 + 0] = isDynamic() ? 1.0 : 0.0;
      attributes[i * VertexAttributesSize
		 + VertexAttributeSizeCoords
		 + VertexAttributeSizeNormal
		 + VertexAttributeSizeTangent
		 + VertexAttributeSizeTextureCoords
		 + VertexAttributeSizeBone
		 + 1] = (GLfloat)meshId;
      attributes[i * VertexAttributesSize
		 + VertexAttributeSizeCoords
		 + VertexAttributeSizeNormal
		 + VertexAttributeSizeTangent
		 + VertexAttributeSizeTextureCoords
		 + VertexAttributeSizeBone
		 + 2] = (GLfloat)materialId;
      
    }
  _attributes->unmap();  
  _needSetIdentifiers = true;
}

void gle::Mesh::setMaterial(gle::Material* material)
{
  _material = material;
}

gle::Material* gle::Mesh::getMaterial() const
{
  return (_material);
}

gle::Buffer<GLuint> * gle::Mesh::getIndexesBuffer() const
{
  return (_indexes);
}

gle::MeshBufferManager::Chunk* gle::Mesh::getAttributes()
{
  if (_isDynamic && _needSetIdentifiers)
    {
      setIdentifiers(0, 0);
      _needSetIdentifiers = false;
    }
  return (_attributes);
}

GLsizeiptr gle::Mesh::getNbIndexes() const
{
  return (_nbIndexes);
}

GLsizeiptr gle::Mesh::getNbVertexes() const
{
  return (_nbVertexes);
}

void gle::Mesh::createBoundingVolume(const GLfloat* datas, GLsizeiptr offset, GLsizeiptr attributeSize, GLsizeiptr nbVertexes)
{
  if (_boundingVolume)
    delete _boundingVolume;
  _boundingVolume = new BoundingBox();
  _boundingVolume->setBestFit(datas, offset, attributeSize, nbVertexes);
}

gle::BoundingVolume* gle::Mesh::getBoundingVolume() const
{
  return (_boundingVolume);
}

const gle::Vector3<GLfloat>& gle::Mesh::getMaxPoint()
{
  if (_needUpdateMatrix)
    {
      this->updateMatrix();
      _needUpdateMatrix = false;
    }
  if (_boundingVolume)
    return (_boundingVolume->getMaxPoint());
  return (_position);
}

const gle::Vector3<GLfloat>& gle::Mesh::getMinPoint()
{
  if (_needUpdateMatrix)
    {
      this->updateMatrix();
      _needUpdateMatrix = false;
    }
  if (_boundingVolume)
    return (_boundingVolume->getMinPoint());
  return (_position);
}

const gle::Vector3<GLfloat>& gle::Mesh::getCenter()
{
  if (_needUpdateMatrix)
    {
      this->updateMatrix();
      _needUpdateMatrix = false;
    }
  if (_boundingVolume)
    return (_boundingVolume->getCenter());
  return (_position);
}

bool gle::Mesh::isInFrustum(const GLfloat frustum[6][4]) const
{
  if (_boundingVolume)
    return (_boundingVolume->isInFrustum(frustum));
  return (true);
}

gle::Scene::Node* gle::Mesh::duplicate() const
{
  return (new Mesh(*this));
}

void gle::Mesh::update()
{
  if (_boundingVolume)
    _boundingVolume->update(_transformationMatrix);
}

void gle::Mesh::setDynamic(bool dynamic, bool deep)
{
  if (dynamic && !_isDynamic)
    {
      _type = gle::Scene::Node::DynamicMesh;
      makeAbsoluteIndexes(false);
      setIdentifiers(0, 0);
    }
  else if (!dynamic && _isDynamic)
    {
      _type = gle::Scene::Node::StaticMesh;
      makeAbsoluteIndexes(true);
    }
  gle::Scene::Node::setDynamic(dynamic, deep);
}

void gle::Mesh::makeAbsoluteIndexes(bool absolute)
{
  if (!_attributes || !_indexes || _absoluteIndexes == absolute)
    return ;
  _absoluteIndexes = absolute;
  GLuint* indexes = _indexes->map();
  GLuint offset = _attributes->getOffset() / VertexAttributesSize;
  for (GLuint i = 0; i < _indexes->getSize(); ++i)
    if (absolute)
      indexes[i] += offset;
    else
      indexes[i] -= offset;
  _indexes->unmap();
}

bool gle::Mesh::canBeRenderedWith(const gle::Scene::MeshGroup& group, bool ignoreBufferId, bool ignoreMaterial) const
{
  return ((_rasterizationMode == group.rasterizationMode)
	  && (ignoreMaterial
	      || _material == NULL
	      || _material->canBeRenderedWith(group))
	  && (ignoreBufferId
	      || (_uniformBufferId == group.uniformBufferId
		  && _materialBufferId == group.materialBufferId)));
}

void gle::Mesh::setUniformBufferId(GLint id)
{
  _uniformBufferId = id;
}

void gle::Mesh::setMaterialBufferId(GLint id)
{
  _materialBufferId = id;
}

GLint gle::Mesh::getUniformBufferId() const
{
  return (_uniformBufferId);
}

GLint gle::Mesh::getMaterialBufferId() const
{
  return (_materialBufferId);
}

std::vector<gle::Scene::Node*>& gle::Mesh::getDebugNodes(int mode)
{
  _debugNodes.clear();
  if (_boundingVolume && (mode & Renderer::BoundingVolume))
    _debugNodes.push_back(_boundingVolume->getDebugMesh(this));
  return (_debugNodes);
}

void gle::Mesh::updateMatrix()
{
  _needUniformsUpdate = true;
  Scene::Node::updateMatrix();
}

const GLfloat* gle::Mesh::getUniforms()
{
  if (_needUniformsUpdate || !_skeleton || _skeletonId != _skeleton->getId())
    {
      if (!_uniforms)
	_uniforms = new GLfloat[UniformSize];
      for (int i = 0; i < UniformSize; ++i)
	_uniforms[i] = 0;
      Matrix4<GLfloat> matrix = getTransformationMatrix();
      GLfloat* floatMatrix = (GLfloat*)matrix;
      for (int i = 0; i < 16; ++i)
	_uniforms[i] = floatMatrix[i];
      if (_skeleton)
	{
	  _uniforms[16] = _skeleton->getId();
	  _skeletonId = _skeleton->getId();
	}
      else
	_uniforms[16] = -1;
      _needUniformsUpdate = false;
    }
  return (_uniforms);
}

void        gle::Mesh::setSkeleton(gle::Skeleton* skeleton)
{
  _skeleton = skeleton;
}

gle::Skeleton*   gle::Mesh::getSkeleton()
{
  return (_skeleton);
}
