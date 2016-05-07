#include "PLUGIN_NAMEInteract.hpp"

using namespace OFX;

template <class T> inline T
Minimum(T a, T b) {    return (a < b) ? a : b;}

template <class T> inline T
Absolute(T a) { return (a < 0) ? -a : a;}


bool PLUGIN_NAMEInteract::draw(const OFX::DrawArgs &args)
{
  OfxRGBColourF col;
  switch(_state) 
  {
    case eInActive: col.r = col.g = col.b = 0.0f; break;
    case ePoised: col.r = col.g = col.b = 0.5f; break;
    case ePicked: col.r = col.g = col.b = 1.0f; break;
  }

  // make the box a constant size on screen by scaling by the pixel scale
  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);

  // Draw a cross hair, the current coordinate system aligns with the image plane.
  glPushMatrix();

  // draw the bo
  glColor3f(col.r, col.g, col.b);
  glTranslated(_position.x, _position.y, 0);
  glBegin(GL_POLYGON);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();

  glPushMatrix();
  // draw a complementary outline
  glColor3f(1.0f - col.r, 1.0f - col.g, 1.0f - col.b);
  glTranslated(_position.x, _position.y, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(-dx, -dy);
  glVertex2f(-dx,  dy);
  glVertex2f( dx,  dy);
  glVertex2f( dx, -dy);
  glEnd();
  glPopMatrix();

  return true;
}

// overridden functions from OFX::Interact to do things
bool PLUGIN_NAMEInteract::penMotion(const OFX::PenArgs &args)
{
  // figure the size of the box in cannonical coords
  float dx = (float)(kBoxSize.x * args.pixelScale.x);
  float dy = (float)(kBoxSize.y * args.pixelScale.y);

  // pen position is in cannonical coords
  OfxPointD penPos = args.penPosition;

  switch(_state) {
    case eInActive: 
    case ePoised: 
    {
      // are we in the box, become 'poised'
      StateEnum newState;
      penPos.x -= _position.x;
      penPos.y -= _position.y;
      if(Absolute(penPos.x) < dx &&
        Absolute(penPos.y) < dy) {
          newState = ePoised;
      }
      else {
        newState = eInActive;
      }

      if(_state != newState) {
        // we have a new state
        _state = newState;

        // and force an overlay redraw
        _effect->redrawOverlays();
      }
    }
    break;
    case ePicked:
    {
      // move our position
      _position = penPos;

      // and force an overlay redraw
      _effect->redrawOverlays();
    }
    break;
  }

  // we have trapped it only if the mouse ain't over it or we are actively dragging
  return _state != eInActive;
}

bool PLUGIN_NAMEInteract::penDown(const OFX::PenArgs &args)
{
  // this will refigure the state
  penMotion(args);

  // if poised means we were over it when the pen went down, so pick it
  if(_state == ePoised) {
    // we are now picked
    _state = ePicked;

    // move our position
    _position = args.penPosition;

    // and request a redraw just incase
    _effect->redrawOverlays();
  }

  return _state == ePicked;
}

bool PLUGIN_NAMEInteract::penUp(const OFX::PenArgs &args)
{
  if(_state == ePicked) {
    // reset to poised for a moment
    _state = ePoised;

    // this will refigure the state
    penMotion(args);

    // and redraw for good measure
    _effect->redrawOverlays();

    // we did trap it
    return true;
  }

  // we didn't trap it
  return false;
}
