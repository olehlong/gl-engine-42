//
// flycam.hpp for  in /home/jochau_g//dev/opengl/gl-engine-42
// 
// Made by gael jochaud-du-plessix
// Login   <jochau_g@epitech.net>
// 
// Started on  Fri Mar 16 10:38:56 2012 gael jochaud-du-plessix
// Last update Thu May 24 22:19:27 2012 gael jochaud-du-plessix
//

#ifndef _FLYCAM_HPP_
# define _FLYCAM_HPP_

# include <SFML/System/Clock.hpp>

namespace flycam {
  extern sf::Clock	timer;
  extern GLfloat	teta;
  extern GLfloat	phi;
  extern std::map<sf::Keyboard::Key, bool> keyState;
  extern GLfloat	mouseX;
  extern GLfloat	mouseY;
  extern GLfloat	moveUp;
  extern GLfloat	mouseSensibility;
  extern GLfloat	camSpeed;

  void event(sf::Event &event, sf::Window & app);
  void flycam(gle::Camera* camera);
};

#endif
