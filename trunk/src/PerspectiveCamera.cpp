//
// PerspectiveCamera.cpp for gle in /home/michar_l//glEngine
// 
// Made by loick michard
// Login   <michar_l@epitech.net>
// 
// Started on  Sat Feb 18 12:53:42 2012 loick michard
// Last update Thu Jun  7 09:43:19 2012 loick michard
//

#include <PerspectiveCamera.hpp>

gle::PerspectiveCamera::PerspectiveCamera(gle::Vector3<GLfloat>
					  const& position,
					  GLfloat fovy,
					  GLfloat aspect,
					  GLfloat near,
					  GLfloat far)
  : gle::Camera(position, position),  _fovy(fovy), _aspect(aspect),
    _near(near), _far(far)
{
  _target += gle::Vector3<GLfloat>(0, 0, 1);
  this->updateMatrix();
}

gle::PerspectiveCamera::PerspectiveCamera(gle::Vector3<GLfloat>
                                          const& position,
					  gle::Vector3<GLfloat>
                                          const& target,
                                          GLfloat fovy,
                                          GLfloat aspect,
                                          GLfloat near,
                                          GLfloat far)
  : gle::Camera(position, target), _fovy(fovy), _aspect(aspect),
    _near(near), _far(far)
{
  this->updateMatrix();
}

void gle::PerspectiveCamera::updateProjectionMatrix()
{
  _projectionMatrix = gle::Matrix4<GLfloat>::perspective(_fovy, _aspect,
							 _near, _far);
}
