#pragma once

#ifdef _WINDOWS
#include <windows.h>
#endif
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "ofxsImageEffect.h"
#include "ofxsInteract.h"

////////////////////////////////////////////////////////////////////////////////
// a dumb interact that just draw's a square you can drag
static const OfxPointD kBoxSize = {20, 20};

class PLUGIN_NAMEInteract : public OFX::OverlayInteract
{
protected:
  enum StateEnum {
    eInActive,
    ePoised,
    ePicked
  };

  OfxPointD _position;
  StateEnum _state;

public:
  PLUGIN_NAMEInteract(OfxInteractHandle handle, OFX::ImageEffect* /*effect*/)
    : OFX::OverlayInteract(handle)
    , _state(eInActive)
  {
    _position.x = 0;
    _position.y = 0;
  }

  // overridden functions from OFX::Interact to do things
  virtual bool draw(const OFX::DrawArgs &args);
  virtual bool penMotion(const OFX::PenArgs &args);
  virtual bool penDown(const OFX::PenArgs &args);
  virtual bool penUp(const OFX::PenArgs &args);
};

class PLUGIN_NAMEOverlayDescriptor : public OFX::DefaultEffectOverlayDescriptor<PLUGIN_NAMEOverlayDescriptor, PLUGIN_NAMEInteract>
{};
