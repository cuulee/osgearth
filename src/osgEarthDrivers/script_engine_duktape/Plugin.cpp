/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2013 Pelican Mapping
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include <osgDB/ReaderWriter>
#include <osgEarthFeatures/ScriptEngine>
#include <osgEarth/Common>
#include <osgDB/FileNameUtils>

#include "DuktapeEngine"


namespace osgEarth { namespace Drivers { namespace Duktape
{
    class DuktapeScriptEngineDriver : public osgEarth::Features::ScriptEngineDriver
    {
    public:
        DuktapeScriptEngineDriver()
        {
            supportsExtension( "osgearth_scriptengine_duktape", "osgEarth Duktape JavaScript Engine" );
        }

        const char* className()
        {
            return "osgEarth Duktape JavaScript Engine";
        }

        ReadResult readObject(const std::string& filename, const osgDB::Options* dbOptions) const
        {
          if ( !acceptsExtension(osgDB::getLowerCaseFileExtension(filename)) )
                return ReadResult::FILE_NOT_HANDLED;

            return ReadResult( new DuktapeScriptEngine(getScriptEngineOptions(dbOptions)) );
        }
    };

    REGISTER_OSGPLUGIN(osgearth_scriptengine_duktape, DuktapeScriptEngineDriver)

} } } // namespace osgEarth::Drivers::Duktape
