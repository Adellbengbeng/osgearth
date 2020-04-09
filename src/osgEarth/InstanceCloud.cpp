/* -*-c++-*- */
/* osgEarth - Geospatial SDK for OpenSceneGraph
* Copyright 2020 Pelican Mapping
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
#include <osgEarth/InstanceCloud>
#include <osgEarth/VirtualProgram>
#include <osg/Program>
#include <osg/GLExtensions>

using namespace osgEarth;

//...................................................................

#define BINDING_COMMAND_BUFFER 0
#define BINDING_POINTS_BUFFER 1
#define BINDING_RENDER_BUFFER 2

namespace
{

    //const char* cull_CS = OE_MULTILINE(
    //    #version 430

    //    layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

    //    struct DrawElementsIndirectCommand {
    //        uint count;
    //        uint instanceCount;
    //        uint firstIndex;
    //        uint baseVertex;
    //        uint baseInstance;
    //    };

    //    layout(std430, binding=0) buffer DrawCommandsBuffer {
    //        DrawElementsIndirectCommand cmd[];
    //    };

    //    layout(std430, binding=1) buffer PointsBuffer {
    //        vec4 points[];
    //    };

    //    layout(std430, binding=2) buffer RenderBuffer {
    //        vec4 render[];
    //    };

    //    bool visible(in vec4 model)
    //    {
    //        vec4 clip = gl_ModelViewProjectionMatrix * model;
    //        clip.xyz = abs(clip.xyz/clip.w);
    //        const float f = 1.0;
    //        return clip.x <= f && clip.y <= f && clip.z <= f;
    //    }

    //    uniform vec2 oe_GroundCover_numInstances;
    //    uniform vec2 oe_GroundCover_LL, oe_GroundCover_UR;

    //    void main()
    //    {
    //        const uint x = gl_GlobalInvocationID.x;
    //        const uint y = gl_GlobalInvocationID.y;

    //        vec2 dim = oe_GroundCover_numInstances;

    //        vec2 offset = vec2(float(x)/dim, float(y)/dim);
    //        vec2 halfSpacing = 0.5/dim;
    //        vec4 tilec = vec4(halfSpacing+offset/dim, 0, 1);

    //        //TODO: noise shift            

    //        vec4 vertex = vec4(mix(oe_GroundCover_LL.xy, oe_GroundCover_UR.xy, tilec.st),0,1);

    //        //TODO: biome cull
    //        //TODO: mask cull
    //        //TODO: elevation

    //        // frustum cull:
    //        if (visible(vertex) {
    //            uint slot = atomicAdd(cmd[0].instanceCount, 1);
    //            render[slot] = vertex;
    //        }
    //    }
    //);

#if 0
    const char* cull_CS =
        "#version 430\n"

        "layout(local_size_x=1, local_size_y=1, local_size_z=1) in; \n"

        "struct DrawElementsIndirectCommand { \n"
        "    uint count; \n"
        "    uint instanceCount; \n"
        "    uint firstIndex; \n"
        "    uint baseVertex; \n"
        "    uint baseInstance; \n"
        "}; \n"

        "layout(std430, binding=0) buffer DrawCommandsBuffer { \n"
        "    DrawElementsIndirectCommand cmd[]; \n"
        "}; \n"

        "layout(std430, binding=1) buffer PointsBuffer { \n"
        "    vec4 points[]; \n"
        "}; \n"

        "layout(std430, binding=2) buffer RenderBuffer { \n"
        "    vec4 render[]; \n"
        "}; \n"

        "bool visible(in vec4 model) \n"
        "{ \n"
        "    vec4 clip = gl_ModelViewProjectionMatrix * model; \n"
        "    clip.xyz = abs(clip.xyz/clip.w); \n"
        "    const float f = 1.0; \n"
        "    return clip.x <= f && clip.y <= f; \n"
        "} \n"

        "void main() { \n"
        "    const uint i = gl_GlobalInvocationID.x; \n"
        "    if (visible(points[i])) // frustum cull \n"
        "    { \n"
        "        uint slot = atomicAdd(cmd[0].instanceCount, 1); \n"
        "        render[slot] = points[i]; \n"
        "    } \n"
        "} \n";
#endif

    const char* cull_CS =
        "#version 430\n"

        "layout(local_size_x=1, local_size_y=1, local_size_z=1) in; \n"

        "struct DrawElementsIndirectCommand { \n"
        "    uint count; \n"
        "    uint instanceCount; \n"
        "    uint firstIndex; \n"
        "    uint baseVertex; \n"
        "    uint baseInstance; \n"
        "}; \n"

        "layout(std430, binding=0) buffer DrawCommandsBuffer { \n"
        "    DrawElementsIndirectCommand cmd[]; \n"
        "}; \n"

        "layout(std430, binding=1) buffer PointsBuffer { \n"
        "    vec4 points[]; \n"
        "}; \n"

        "layout(std430, binding=2) buffer RenderBuffer { \n"
        "    vec4 render[]; \n"
        "}; \n"

        "bool visible(in vec4 model) \n"
        "{ \n"
        "    vec4 clip = gl_ModelViewProjectionMatrix * model; \n"
        "    clip.xyz = abs(clip.xyz/clip.w); \n"
        "    const float f = 1.0; \n"
        "    return clip.x <= f && clip.y <= f; \n"
        "} \n"

        "uniform vec2 oe_GroundCover_numInstances; \n"
        "uniform vec2 oe_GroundCover_LL; \n"
        "uniform vec2 oe_GroundCover_UR; \n"

        "void main() { \n"
        "    const uint x = gl_GlobalInvocationID.x; \n"
        "    const uint y = gl_GlobalInvocationID.y; \n"

        "    vec2 dim = oe_GroundCover_numInstances; \n"

        "    vec2 offset = vec2(float(x), float(y)); \n"
        "    vec2 halfSpacing = 0.5/dim; \n"
        "    vec2 tilec = halfSpacing + offset/dim; \n"

        //"    //TODO: noise shift \n"

        "    vec4 vertex = vec4(mix(oe_GroundCover_LL.xy, oe_GroundCover_UR.xy, tilec),0,1); \n"

        //"    //TODO: biome cull \n"
        //"    //TODO: mask cull \n"
        //"    //TODO: elevation \n"

        //"    if (visible(vertex)) // frustum cull \n"
        "    { \n"
        "        uint slot = atomicAdd(cmd[0].instanceCount, 1); \n"
        "        render[slot] = vertex; \n"
        "    } \n"
        "} \n";

    const char* oe_IC_renderVS =
        "#version 430 \n"

        "layout(std430, binding=2) buffer RenderBuffer { \n"
        "    vec4 render[]; \n"
        "}; \n"

        "void oe_IC_renderVS(inout vec4 vertex) { \n"
        "    vertex.xyz += render[gl_InstanceID].xyz; \n"
        "} \n";
}

//...................................................................

InstanceCloud::DrawElementsIndirectCommand::DrawElementsIndirectCommand() :
    count(0),
    instanceCount(0),
    firstIndex(0),
    baseVertex(0),
    baseInstance(0)
{
    //nop
}

InstanceCloud::InstancingData::InstancingData() :
    points(NULL),
    commandBuffer(-1),
    pointsBuffer(-1),
    renderBuffer(-1)
{
    //nop
}

bool
InstanceCloud::InstancingData::needsAllocate() const
{
    return commandBuffer == -1;
}

void
InstanceCloud::InstancingData::allocate(osg::State* state)
{
    GLuint numInstances = points ? points->size() : numX*numY;
    GLuint instanceSize = sizeof(GLfloat)*4;

    osg::GLExtensions* ext = state->get<osg::GLExtensions>();

    ext->glGenBuffers(1, &commandBuffer);
    ext->glBindBuffer(GL_SHADER_STORAGE_BUFFER, commandBuffer);
    ext->glBufferStorage(
        GL_SHADER_STORAGE_BUFFER,
        1 * sizeof(DrawElementsIndirectCommand),
        &command,
        GL_DYNAMIC_STORAGE_BIT);

    // buffer for the input data:
    ext->glGenBuffers(1, &pointsBuffer);
    ext->glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointsBuffer);
    ext->glBufferStorage(
        GL_SHADER_STORAGE_BUFFER, 
        (numInstances * instanceSize), 
        points ? points->getDataPointer() : NULL,
        0);

    // buffer for the output data (culled points, written by compute shader)
    ext->glGenBuffers(1, &renderBuffer);
    ext->glBindBuffer(GL_SHADER_STORAGE_BUFFER, renderBuffer);
    ext->glBufferStorage(
        GL_SHADER_STORAGE_BUFFER, 
        (numInstances * instanceSize), 
        NULL, 
        0);
}

InstanceCloud::InstanceCloud()
{
    // install our compute shader in a stateset
    _computeStateSet = new osg::StateSet();
    osg::Shader* s = new osg::Shader(osg::Shader::COMPUTE, cull_CS);
    osg::Program* p = new osg::Program();
    p->addShader(s);
    _computeStateSet->setAttribute(p, osg::StateAttribute::ON);
}

void
InstanceCloud::setGeometry(osg::Geometry* geom)
{
    _geom = geom;

    if (geom)
    {
        Installer installer(&_data);
        geom->accept(installer);
    }
}

void
InstanceCloud::setPositions(osg::Vec4Array* value)
{
    _data.points = value;
}

void
InstanceCloud::setNumInstances(unsigned x, unsigned y)
{
    _data.numX = x;
    _data.numY = y;
}

osg::BoundingBox
InstanceCloud::computeBoundingBox() const
{
    osg::BoundingBox box;
    float radius = _geom.valid() ? _geom->getBound().radius() : 0.0f;
    osg::Vec3 radius3(radius, radius, radius);
    if (_data.points)
    {
        for(int i=0; i<_data.points->size(); ++i)
        {
            const osg::Vec4& v = (*_data.points)[i];
            osg::Vec3 center(v.x()/v.w(), v.y()/v.w(), v.z()/v.w());
            osg::BoundingBox pbox(center-radius3, center+radius3);
            box.expandBy(pbox);
        }
    }
    return box;
}

const osg::Program::PerContextProgram* lastPCP;

void
InstanceCloud::bind(osg::RenderInfo& ri)
{
    osg::State* state = ri.getState();

    // allocate GL objects on first run
    if (_data.needsAllocate())
        _data.allocate(state);

    // save the last known program so we can restore it after running compute shaders
    lastPCP = state->getLastAppliedProgramObject();

    //activate compute shader
    state->apply(_computeStateSet.get());
}

void
InstanceCloud::cull(osg::RenderInfo& ri)
{
    //if (_data.points == NULL || _data.points->empty())
    //    return;

    osg::State* state = ri.getState();

    //// allocate GL objects on first run
    //if (_data.needsAllocate())
    //    _data.allocate(state);

    //// save the last known program so we can restore it after running compute shaders
    //const osg::Program::PerContextProgram* lastPCP = state->getLastAppliedProgramObject();

    ////activate compute shader
    //state->apply(_computeStateSet.get());

    if (state->getUseModelViewAndProjectionUniforms()) 
        state->applyModelViewAndProjectionUniformsIfRequired();

    osg::GLExtensions* ext = state->get<osg::GLExtensions>();

    // Clear out the instance count:
    ext->glBindBuffer(GL_SHADER_STORAGE_BUFFER, _data.commandBuffer);
    ext->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(DrawElementsIndirectCommand), &_data.command);
    //ext->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // necessary?

    ext->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_COMMAND_BUFFER, _data.commandBuffer);
    ext->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_POINTS_BUFFER, _data.pointsBuffer);
    ext->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_RENDER_BUFFER, _data.renderBuffer);

    //ext->glDispatchCompute(_data.points->size(), 1, 1);
    ext->glDispatchCompute(_data.numX, _data.numY, 1); //_data.points->size(), 1, 1);

    ext->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

    // restore previous shader program
    //state->popStateSet();
    state->apply();
    if (lastPCP)
    {
        lastPCP->useProgram();
        state->setLastAppliedProgramObject(lastPCP);
    }
}

void
InstanceCloud::draw(osg::RenderInfo& ri) 
{
    _geom->draw(ri);
}


InstanceCloud::Renderer::Renderer(InstancingData* data) :
    _data(data)
{
    //nop
}

void
InstanceCloud::Renderer::drawImplementation(osg::RenderInfo& ri, const osg::Drawable* drawable) const
{
    osg::State& state = *ri.getState();

    osg::GLExtensions* ext = state.get<osg::GLExtensions>();

    osg::VertexArrayState* vas = state.getCurrentVertexArrayState();
    vas->setVertexBufferObjectSupported(true);

    const osg::Geometry* geom = drawable->asGeometry();
    geom->drawVertexArraysImplementation(ri);

    // TODO: support multiple primtsets....?
    osg::GLBufferObject* ebo = geom->getPrimitiveSet(0)->getOrCreateGLBufferObject(state.getContextID());
    state.getCurrentVertexArrayState()->bindElementBufferObject(ebo);

    ext->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_RENDER_BUFFER, _data->renderBuffer);

    ext->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _data->commandBuffer);
    ext->glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0, 1, 0);
    //ext->glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    ext->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_RENDER_BUFFER, 0);
}


InstanceCloud::Installer::Installer(InstancingData* data) :
    osg::NodeVisitor(TRAVERSE_ALL_CHILDREN),
    _data(data)
{
    _callback = new Renderer(data);
}

void 
InstanceCloud::Installer::apply(osg::Drawable& drawable)
{
    osg::Geometry* geom = drawable.asGeometry();
    if (geom)
    {
        geom->setDrawCallback(_callback.get());
        geom->setCullingActive(false);
        _data->command.count = geom->getPrimitiveSet(0)->getNumIndices();
    }
}

//...................................................................

InstanceCloudDrawable::InstanceCloudDrawable()
{
    setCullingActive(false);
    _cloud = new InstanceCloud();

    VirtualProgram* vp = VirtualProgram::getOrCreate(getOrCreateStateSet());
    vp->setFunction("oe_IC_renderVS", oe_IC_renderVS, ShaderComp::LOCATION_VERTEX_MODEL, -FLT_MAX);
}

osg::BoundingBox
InstanceCloudDrawable::computeBoundingBox() const
{
    return _cloud->computeBoundingBox();
}

void 
InstanceCloudDrawable::drawImplementation(osg::RenderInfo& ri) const
{
    _cloud->cull(ri);
    _cloud->draw(ri);
}

#if 0

#include <osgViewer/Viewer>
#include <osgEarth/ExampleResources>
#include <osgEarth/AnnotationUtils>
#include <osgEarth/InstanceCloud>
#include <osgEarth/NodeUtils>

#define DIM 100
#define SPACING 3

osg::Geometry* makeInstance()
{
    return osgEarth::findTopMostNodeOfType<osg::Geometry>(
        AnnotationUtils::createSphere(-0.5, osg::Vec4(1, .5, 0, 1), 15.0));
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    osgViewer::Viewer viewer(arguments);
    osg::Group* group = new osg::Group();

    osg::Vec4Array* points = new osg::Vec4Array();
    for (int i = 0; i < DIM; ++i)
        for (int j = 0; j < DIM; ++j)
            points->push_back(osg::Vec4(SPACING * (double)i, SPACING * (double)j, 0, 1));

    osg::Geometry* geom = makeInstance();

    InstanceCloudDrawable* drawable = new InstanceCloudDrawable();
    drawable->cloud()->setGeometry(geom);
    drawable->cloud()->setPositions(points);
    group->addChild(drawable);

    MapNodeHelper().parse(NULL, arguments, &viewer, group, (Controls::Container*)NULL);
    MapNodeHelper().configureView(&viewer);

    viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    viewer.getCamera()->setProjectionMatrixAsPerspective(30.0, 1.0, 1.0, 1e6);

    viewer.setSceneData(group);
    return viewer.run();
}

#endif
