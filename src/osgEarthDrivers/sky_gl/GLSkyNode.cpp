/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
* Copyright 2015 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "GLSkyNode"
#include <osg/LightSource>

#include <osgEarth/VirtualProgram>
#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>
#include <osgEarth/Lighting>
#include <osgEarth/PhongLightingEffect>
#include <osgEarthUtil/Ephemeris>

#define LC "[GLSkyNode] "

using namespace osgEarth;
using namespace osgEarth::Util;
using namespace osgEarth::GLSky;

//---------------------------------------------------------------------------

GLSkyNode::GLSkyNode(const Profile* profile) :
SkyNode()
{
    initialize(profile);
}

GLSkyNode::GLSkyNode(const Profile*      profile,
                     const GLSkyOptions& options) :
SkyNode ( options ),
_options( options )
{
    initialize(profile);
}

void
GLSkyNode::initialize(const Profile* profile)
{
    _profile = profile;
    //_light = new osg::Light(0);
    _light = new LightGL3(0);
    _light->setDataVariance(_light->DYNAMIC);
    _light->setAmbient(osg::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
    _light->setDiffuse(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    _light->setSpecular(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    if ( _options.ambient().isSet() )
    {
        float a = osg::clampBetween(_options.ambient().get(), 0.0f, 1.0f);
        _light->setAmbient(osg::Vec4(a, a, a, 1.0f));
    }

    // installs the main uniforms and the shaders that will light the subgraph (terrain).
    osg::StateSet* stateset = this->getOrCreateStateSet();

    _lighting = new PhongLightingEffect();
    _lighting->setCreateLightingUniform( false );
    _lighting->attach( stateset );

    // install the Sun as a lightsource.
    osg::LightSource* lightSource = new osg::LightSource();
    lightSource->setLight(_light.get());
    lightSource->setCullingActive(false);
    this->addChild( lightSource );
    lightSource->addCullCallback(new LightSourceGL3UniformGenerator());

    onSetDateTime();
}

GLSkyNode::~GLSkyNode()
{
    if ( _lighting.valid() )
        _lighting->detach();
}

void
GLSkyNode::onSetEphemeris()
{
    // trigger the date/time update.
    onSetDateTime();
}

void
GLSkyNode::onSetReferencePoint()
{
    onSetDateTime();
}

void
GLSkyNode::onSetDateTime()
{
    if ( !getSunLight() || !_profile.valid() )
        return;

    const DateTime& dt = getDateTime();
    osg::Vec3d sunPosECEF = getEphemeris()->getSunPositionECEF( dt );

    if ( _profile->getSRS()->isGeographic() )
    {
        // Convert a Vec3d into a Vec4f without losing precision.
        // This is critical if we are going to convert the position to View space
        // and pass it in a uniform.
        osg::Vec4 pos(sunPosECEF, 1.0);
        while (osg::Vec3(pos.x(), pos.y(), pos.z()).length() > 1000000.0f)
            pos *= 0.1f;

        _light->setPosition(pos);
    }
    else
    {
        // pull the ref point:
        GeoPoint refpoint = getReferencePoint();
        if ( !refpoint.isValid() )
        {
            // not found; use the center of the profile:
            _profile->getExtent().getCentroid(refpoint);
        }

        // convert to lat/long:
        GeoPoint refLatLong;
        refpoint.transform(_profile->getSRS()->getGeographicSRS(), refLatLong);

        // Matrix to convert the ECEF sun position to the local tangent plane
        // centered on our reference point:
        osg::Matrixd world2local;
        refLatLong.createWorldToLocal(world2local);

        // convert the sun position:
        osg::Vec3d sunPosLocal = sunPosECEF * world2local;
        sunPosLocal.normalize();

        getSunLight()->setPosition( osg::Vec4(sunPosLocal, 0.0) );
    }
}

void
GLSkyNode::attach( osg::View* view, int lightNum )
{
    if ( !view ) return;

    _light->setLightNum( lightNum );

    //OE_INFO << LC << "Attaching light to view" << std::endl;

    // Tell the view not to automatically include a light.
    view->setLightingMode( osg::View::NO_LIGHT );

    // install or convert the default material for this view.
    osg::StateSet* camSS = view->getCamera()->getOrCreateStateSet();
    osg::Material* material = dynamic_cast<osg::Material*>(camSS->getAttribute(osg::StateAttribute::MATERIAL));
    material = material ? new MaterialGL3(*material) : new MaterialGL3();

    // Set up some default material properties.
    material->setDiffuse(material->FRONT, osg::Vec4(1,1,1,1));
    // Set ambient reflectance to 1 so that ambient light is in control:
    material->setAmbient(material->FRONT, osg::Vec4(1,1,1,1));

    osg::Uniform* numLights = camSS->getOrCreateUniform("osg_NumLights", osg::Uniform::INT);
    int value = 0;
    numLights->get(value);
    numLights->set(value+1);

    // install the replacement:
    camSS->setAttribute(material);

    // Create static uniforms for this material.
    MaterialGL3UniformGenerator().generate(camSS, material);

    // initial date/time setup.
    onSetDateTime();
}
