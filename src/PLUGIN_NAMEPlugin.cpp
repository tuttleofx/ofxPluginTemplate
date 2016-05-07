
#include <stdio.h>
#include <cassert>
#include <algorithm>

#include "ofxsImageEffect.h"

#include "ImageProcessor.hpp"
#include "PLUGIN_NAMEInteract.hpp"


template <class T> inline T 
Clamp(T v, int min, int max)
{
  if(v < T(min)) return T(min);
  if(v > T(max)) return T(max);
  return v;
}

// Base class for the RGBA and the Alpha processor
class ImageScalerBase : public OFX::ImageProcessor {
protected :
  OFX::Image *_srcImg;
  OFX::Image *_maskImg;
  double _rScale, _gScale, _bScale, _aScale;
  bool   _doMasking;

public :
  /** @brief no arg ctor */
  ImageScalerBase(OFX::ImageEffect &instance)
    : OFX::ImageProcessor(instance)
    , _srcImg(0)
    , _maskImg(0)
    , _rScale(1)
    , _gScale(1)
    , _bScale(1)
    , _aScale(1)
    , _doMasking(false)
  {        
  }

  /** @brief set the src image */
  void setSrcImg(OFX::Image *v) {_srcImg = v;}

  /** @brief set the optional mask image */
  void setMaskImg(OFX::Image *v) {_maskImg = v;}

  // Are we masking. We can't derive this from the mask image being set as NULL is a valid value for an input image
  void doMasking(bool v) {_doMasking = v;}

  /** @brief set the scale */
  void setScales(float r, float g, float b, float a)
  {
    _rScale = r;
    _gScale = g;
    _bScale = b;
    _aScale = a;
  }

};

// template to do the RGBA processing
template <class PIX, int nComponents, int max>
class ImageScaler : public ImageScalerBase {
public :
  // ctor
  ImageScaler(OFX::ImageEffect &instance) 
    : ImageScalerBase(instance)
  {}

  // and do some processing
  void multiThreadProcessImages(OfxRectI procWindow)
  {
    float scales[4];
    scales[0] = nComponents == 1 ? (float)_aScale : (float)_rScale;
    scales[1] = (float)_gScale;
    scales[2] = (float)_bScale;
    scales[3] = (float)_aScale;

    float maskScale = 1.0f;

    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(_effect.abort()) break;

      PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {

        PIX *srcPix = (PIX *)  (_srcImg ? _srcImg->getPixelAddress(x, y) : 0);

        // are we doing masking
        if(_doMasking) {
          // we do, get the pixel from the mask
          if(!_maskImg)
            maskScale = 1.0f;
          else
          {
            PIX *maskPix = (PIX *)  (_maskImg ? _maskImg->getPixelAddress(x, y) : 0);
            // figure the scale factor from that pixel
            maskScale = maskPix != 0 ? float(*maskPix)/float(max) : 0.0f;
          }
        }

        // do we have a source image to scale up
        if(srcPix) {
          for(int c = 0; c < nComponents; c++) {
            float v;

            // scale the component up by the scale factor, modulated by the maskScale
            if(maskScale != 1.0f) 
              v = srcPix[c] * (1.0f + (scales[c] - 1.0f) * maskScale);
            else
              v = srcPix[c] * scales[c];

            if(max == 1)  // implies floating point and so no clamping
              dstPix[c] = PIX(v);
            else  // integer based and we need to clamp
              dstPix[c] = PIX(Clamp(v, 0, max));
          }
        }
        else {
          // no src pixel here, be black and transparent
          for(int c = 0; c < nComponents; c++) {
            dstPix[c] = 0;
          }
        }
        // increment the dst pixel
        dstPix += nComponents;
      }
    }
  }
};

/** @brief The plugin that does our work */
class PLUGIN_NAMEPlugin : public OFX::ImageEffect {
protected:
  // do not need to delete these, the ImageEffect is managing them for us
  OFX::Clip *dstClip_;
  OFX::Clip *srcClip_;
  OFX::Clip *maskClip_;

  OFX::DoubleParam  *scale_;
  OFX::DoubleParam  *rScale_;
  OFX::DoubleParam  *gScale_;
  OFX::DoubleParam  *bScale_;
  OFX::DoubleParam  *aScale_;
  OFX::BooleanParam *componentScalesEnabled_;

public:
  /** @brief ctor */
  PLUGIN_NAMEPlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , scale_(0)
    , rScale_(0)
    , gScale_(0)
    , bScale_(0)
    , aScale_(0)
    , componentScalesEnabled_(0)
  {
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    // name of mask clip depends on the context
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    scale_   = fetchDoubleParam("scale");
    rScale_  = fetchDoubleParam("scaleR");
    gScale_  = fetchDoubleParam("scaleG");
    bScale_  = fetchDoubleParam("scaleB");
    aScale_  = fetchDoubleParam("scaleA");
    componentScalesEnabled_ = fetchBooleanParam("scaleComponents");

    // set the enabledness of our RGBA sliders
    setEnabledness();
  }

  /* sets the enabledness of the component scale params depending on the type of input image and the state of the scaleComponents param */
  void setEnabledness(void);

  /* Override the render */
  virtual void render(const OFX::RenderArguments &args);

  /* override is identity */
  virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime);

  /* override changedParam */
  virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName);

  /* override changed clip */
  virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName);

  // override the rod call
  virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod);

  // override the roi call
  virtual void getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois);

  /* set up and run a processor */
  void setupAndProcess(ImageScalerBase &, const OFX::RenderArguments &args);
};


/// set up and run a processor
void PLUGIN_NAMEPlugin::setupAndProcess(ImageScalerBase &processor, const OFX::RenderArguments &args)
{
  // get a dst image
  std::auto_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
  OFX::BitDepthEnum dstBitDepth       = dst->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();

  // fetch main input image
  std::auto_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

  // make sure bit depths are sane
  if(src.get()) {
    OFX::BitDepthEnum    srcBitDepth      = src->getPixelDepth();
    OFX::PixelComponentEnum srcComponents = src->getPixelComponents();

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcComponents != dstComponents)
      throw int(1); // HACK!! need to throw an sensible exception here!
  }

  // auto ptr for the mask.
  // Should do this inside the if statement below but the MS compiler I have doesn't have
  // a 'reset' function on the auto_ptr class
  std::auto_ptr<OFX::Image> mask(getContext() != OFX::eContextFilter ? maskClip_->fetchImage(args.time) : 0);

  // do we do masking
  if(getContext() != OFX::eContextFilter) {
    // say we are masking
    processor.doMasking(true);

    // Set it in the processor 
    processor.setMaskImg(mask.get());
  }

  // get the scale parameter values...
  double r, g, b, a = aScale_->getValueAtTime(args.time);
  r = g = b = scale_->getValueAtTime(args.time);

  // see if the individual component scales are enabled
  if(componentScalesEnabled_->getValueAtTime(args.time)) {
    r *= rScale_->getValueAtTime(args.time);
    g *= gScale_->getValueAtTime(args.time);
    b *= bScale_->getValueAtTime(args.time);
  }

  // set the images
  processor.setDstImg(dst.get());
  processor.setSrcImg(src.get());

  // set the render window
  processor.setRenderWindow(args.renderWindow);

  // set the scales
  processor.setScales((float)r, (float)g, (float)b, (float)a);

  // Call the base class process member, this will call the derived templated process code
  processor.process();
}

// override the rod call
bool PLUGIN_NAMEPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
  // our RoD is the same as the 'Source' clip's, we are not interested in the mask
  rod = srcClip_->getRegionOfDefinition(args.time);

  // say we set it
  return true;
}

// override the roi call
void PLUGIN_NAMEPlugin::getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois)
{
  // we don't actually need to do this as this is the default, but do it for examples sake
  rois.setRegionOfInterest(*srcClip_, args.regionOfInterest);

  // set it on the mask only if we are in an interesting context
  if(getContext() != OFX::eContextFilter)
    rois.setRegionOfInterest(*maskClip_, args.regionOfInterest);
}

// the overridden render function
void PLUGIN_NAMEPlugin::render(const OFX::RenderArguments &args)
{
  // instantiate the render code based on the pixel depth of the dst clip
  OFX::BitDepthEnum       dstBitDepth    = dstClip_->getPixelDepth();
  OFX::PixelComponentEnum dstComponents  = dstClip_->getPixelComponents();

  // do the rendering
  if(dstComponents == OFX::ePixelComponentRGBA) {
    switch(dstBitDepth) {
      case OFX::eBitDepthUByte : {      
        ImageScaler<unsigned char, 4, 255> fred(*this);
        setupAndProcess(fred, args);
        }
        break;

      case OFX::eBitDepthUShort : {
        ImageScaler<unsigned short, 4, 65535> fred(*this);
        setupAndProcess(fred, args);
        }                          
        break;

      case OFX::eBitDepthFloat : {
        ImageScaler<float, 4, 1> fred(*this);
        setupAndProcess(fred, args);
        }
        break;
      default :
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
          }
        }
  else {
    switch(dstBitDepth) {
      case OFX::eBitDepthUByte : {
        ImageScaler<unsigned char, 1, 255> fred(*this);
        setupAndProcess(fred, args);
        }
        break;

      case OFX::eBitDepthUShort : {
        ImageScaler<unsigned short, 1, 65535> fred(*this);
        setupAndProcess(fred, args);
        }                          
        break;

      case OFX::eBitDepthFloat : {
        ImageScaler<float, 1, 1> fred(*this);
        setupAndProcess(fred, args);
        }                          
        break;
      default :
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }
  }
}

// overridden is identity
bool PLUGIN_NAMEPlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime)
{
  // get the scale parameters
  double scale = scale_->getValueAtTime(args.time);
  double rScale = 1, gScale = 1, bScale = 1, aScale = 1;
  if(componentScalesEnabled_->getValueAtTime(args.time)) {
    rScale = rScale_->getValueAtTime(args.time);
    gScale = gScale_->getValueAtTime(args.time);
    bScale = bScale_->getValueAtTime(args.time);
    aScale = aScale_->getValueAtTime(args.time);
  }
  rScale *= scale; gScale *= scale; bScale *= scale;

  // do we do any scaling ?
  if(rScale == 1 && gScale == 1 && bScale == 1 && aScale == 1) {
    identityClip = srcClip_;
    identityTime = args.time;
    return true;
  }

  // nope, idenity we is
  return false;
}

// set the enabledness of the individual component scales
void PLUGIN_NAMEPlugin::setEnabledness(void)
{
  // the componet enabledness depends on the clip being RGBA and the param being true
  bool v = componentScalesEnabled_->getValue() && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA;

  // enable them
  rScale_->setEnabled(v);
  gScale_->setEnabled(v);
  bScale_->setEnabled(v);
  aScale_->setEnabled(v);
}

// we have changed a param
void PLUGIN_NAMEPlugin::changedParam(const OFX::InstanceChangedArgs &/*args*/, const std::string &paramName)
{
  if(paramName == "scaleComponents")
    setEnabledness(); 
}

// we have changed a param
void PLUGIN_NAMEPlugin::changedClip(const OFX::InstanceChangedArgs &/*args*/, const std::string &clipName)
{
  if(clipName == kOfxImageEffectSimpleSourceClipName)
    setEnabledness();
}


using namespace OFX;

mDeclarePluginFactory(PLUGIN_NAMEPluginFactory, {}, {});

void PLUGIN_NAMEPluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
  // basic labels
  desc.setLabels(
    "PLUGIN_NAME",
    "PLUGIN_LABEL",
    "PLUGIN_LONG_LABEL");
  desc.setPluginGrouping("PLUGIN_GROUPING");

  desc.setPluginDescription("PLUGIN_DESCRIPTION");

  // add the supported contexts, only filter at the moment
  desc.addSupportedContext(eContextFilter);
  desc.addSupportedContext(eContextGeneral);
  desc.addSupportedContext(eContextPaint);

  // add supported pixel depths
  desc.addSupportedBitDepth(eBitDepthUByte);
  desc.addSupportedBitDepth(eBitDepthUShort);
  desc.addSupportedBitDepth(eBitDepthFloat);

  // set a few flags
  desc.setSingleInstance(false);
  desc.setHostFrameThreading(false);
  desc.setSupportsMultiResolution(true);
  desc.setSupportsTiles(true);
  desc.setTemporalClipAccess(false);
  desc.setRenderTwiceAlways(false);
  desc.setSupportsMultipleClipPARs(false);

  desc.setOverlayInteractDescriptor( new PLUGIN_NAMEOverlayDescriptor);
}

// make a double scale param
static
DoubleParamDescriptor *defineScaleParam(OFX::ImageEffectDescriptor &desc,
                                        const std::string &name, const std::string &label, const std::string &hint,
                                        GroupParamDescriptor *parent)
{
  DoubleParamDescriptor *param = desc.defineDoubleParam(name);
  param->setLabels(label, label, label);
  param->setScriptName(name);
  param->setHint(hint);
  param->setDefault(1);
  param->setRange(0, 10);
  param->setIncrement(0.1);
  param->setDisplayRange(0, 10);
  param->setDoubleType(eDoubleTypeScale);
  if(parent)
    param->setParent(*parent);
  return param;
}


void PLUGIN_NAMEPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
  // Source clip only in the filter context
  // create the mandated source clip
  ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
  srcClip->addSupportedComponent(ePixelComponentRGBA);
  srcClip->addSupportedComponent(ePixelComponentAlpha);
  srcClip->setTemporalClipAccess(false);
  srcClip->setSupportsTiles(true);
  srcClip->setIsMask(false);

  // if general or paint context, define the mask clip
  if(context == eContextGeneral || context == eContextPaint) {                
    // if paint context, it is a mandated input called 'brush'
    ClipDescriptor *maskClip = context == eContextGeneral ? desc.defineClip("Mask") : desc.defineClip("Brush");
    maskClip->addSupportedComponent(ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    if(context == eContextGeneral)
      maskClip->setOptional(true);
    maskClip->setSupportsTiles(true); 
    maskClip->setIsMask(true); // we are a mask input
  }

  // create the mandated output clip
  ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
  dstClip->addSupportedComponent(ePixelComponentRGBA);
  dstClip->addSupportedComponent(ePixelComponentAlpha);
  dstClip->setSupportsTiles(true);

  // make some pages and to things in 
  PageParamDescriptor *page = desc.definePageParam("Controls");

  // group param to group the scales
  GroupParamDescriptor *componentScalesGroup = desc.defineGroupParam("componentScales");
  componentScalesGroup->setHint("Scales on the individual component");
  componentScalesGroup->setLabels("Components", "Components", "Components");            

  // make overall scale params 
  DoubleParamDescriptor *param = defineScaleParam(desc, "scale", "scale", "Scales all component in the image", 0);
  page->addChild(*param);

  // add a boolean to enable the component scale
  BooleanParamDescriptor *boolP = desc.defineBooleanParam("scaleComponents");
  boolP->setDefault(true);
  boolP->setHint("Enables scales on individual components");
  boolP->setLabels("Scale Components", "Scale Components", "Scale Components");
  boolP->setParent(*componentScalesGroup);
  page->addChild(*boolP);

  // make the four component scale params 
  param = defineScaleParam(desc, "scaleR", "red", "Scales the red component of the image", componentScalesGroup);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleG", "green", "Scales the green component of the image", componentScalesGroup);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleB", "blue", "Scales the blue component of the image", componentScalesGroup);
  page->addChild(*param);

  param = defineScaleParam(desc, "scaleA", "alpha", "Scales the alpha component of the image", componentScalesGroup);
  page->addChild(*param);
}

ImageEffect *PLUGIN_NAMEPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
  return new PLUGIN_NAMEPlugin(handle);
}

namespace OFX {
namespace Plugin {
void getPluginIDs(OFX::PluginFactoryArray &ids)
{
  static PLUGIN_NAMEPluginFactory p("PLUGIN_UID", 1, 0);
  ids.push_back(&p);
}  
}
}
