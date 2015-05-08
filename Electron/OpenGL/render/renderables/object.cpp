//
//  render.h
//  
//
//  Created by Samuco on 19/04/2015.
//
//

#include "object.h"
#include "objects/scen.h"
#include "objects/vehi.h"
#include "objects/itmc.h"

ObjectRef::ObjectRef(ObjectManager *manager, ProtonMap *map, HaloTagDependency tag) {
    printf("creating object ref\n");
    TagID = tag;
    
    if (tag.tag_id.tag_index != NULLED_TAG_ID) {
        ProtonTag *objectTag = map->tags.at(tag.tag_id.tag_index).get();
        HaloObjectTagData *objectData = (HaloObjectTagData *)objectTag->Data();
        
        printf("talking to models\n");
        model = manager->modelManager->create_model(map, objectData->model);
    } else {
        printf("null model\n");
        model = nullptr;
    }
}
void ObjectRef::render(ShaderType pass) {
    model->render(pass);
}

ObjectRef *ObjectManager::create_object(ProtonMap *map, HaloTagDependency tag) {
    fprintf(stderr, "creating object %d %c%c%c%c\n", tag.tag_id.tag_index, tag.tag_class[3], tag.tag_class[2], tag.tag_class[1], tag.tag_class[0]);
    
    // Has this bitmap been loaded before? Check the cache
    std::map<uint16_t, ObjectRef*>::iterator iter = this->objects.find(tag.tag_id.tag_index);
    if (iter != this->objects.end()) {
        return iter->second;
    }
    
    // Create a new texture
    ObjectRef *obj = new ObjectRef(this, map, tag);
    this->objects[tag.tag_id.tag_index] = obj;
    return obj;
}

ObjectClass *ObjectManager::getClass(SelectionType type) {
    switch (type) {
        case s_scenery:
            return scen;
            break;
        case s_vehicle:
            return vehi;
            break;
        case s_item:
            return itmc;
            break;
        default:
            break;
    }
    return nullptr;
}

void ObjectManager::read(ShaderManager *shaders, ProtonMap *map, ProtonTag *scenario) {
    printf("starting object manager\n");
    selection = std::vector<ObjectInstance*>();
    modelManager = new ModelManager(shaders);
    scen = new ScenClass;
    scen->read(this, map, scenario);
    vehi = new VehiClass;
    vehi->read(this, map, scenario);
    itmc = new ItmcClass;
    itmc->read(this, map, scenario);
    
    // Store the map and scenario for bsp lookup
    this->map = map;
    this->scenario = scenario;
}

void ObjectManager::write(ProtonMap *map, ProtonTag *scenario) {
    scen->write(map, scenario);
    vehi->write(map, scenario);
    itmc->write(map, scenario);
}

void ObjectManager::render_instance(ObjectInstance *instance, ShaderType pass) {
    // Frustrum culling

    // Culling
    //vector3d *position = new vector3d(instance->x, instance->y, instance->z);
    //bsp->intersect(camera->position, position, map, scenario); too slow 
    //delete position;
    
    // Render
    if (instance->selected) {
        // Render a cross as the coordinates. TODO: Replace with non fixed pipeline
        if (pass == shader_NULL) {
            float lineLen = 100.0;
            float largeFloat = 1000000.0;
            vector3d *position      = new vector3d(instance->x, instance->y, instance->z);
            vector3d *position_down = new vector3d(instance->x, instance->y, instance->z-largeFloat);
            vector3d *intersect = bsp->intersect(position_down, position, map, scenario);

            // Is this object below the BSP? Move it above automatically
            glEnable(GL_COLOR_MATERIAL);
            if (intersect == nullptr) {
                glColor4f(1.0, 0.0, 0.0, 1.0);
            } else {
                delete intersect;
                glColor4f(1.0, 1.0, 1.0, 1.0);
            }
            delete position;
            delete position_down;
            
            glLineWidth(1.0);
            glBegin(GL_LINES);
            glVertex3f(instance->x-lineLen, instance->y, instance->z);
            glVertex3f(instance->x+lineLen, instance->y, instance->z);
            glVertex3f(instance->x, instance->y-lineLen, instance->z);
            glVertex3f(instance->x, instance->y+lineLen, instance->z);
            glVertex3f(instance->x, instance->y, instance->z-lineLen);
            glVertex3f(instance->x, instance->y, instance->z+lineLen);
            glEnd();
        }
    }
    
    glPushMatrix();
    glTranslatef(instance->x, instance->y, instance->z);
    glRotatef(instance->roll , 1.0, 0.0, 0.0); //1.5%
    glRotatef(instance->pitch, 0.0, 1.0, 0.0); //0.9%
    glRotatef(instance->yaw  , 0.0, 0.0, 1.0); //1.2%
    instance->reference->render(pass);
    glPopMatrix();
    
    if (instance->selected) {
        glColor4f(1.0, 1.0, 1.0, 1.0);
    }
}

void ObjectManager::render_subclass(ObjectClass* objClass, SelectionType selection, GLuint *name, GLuint *lookup, ShaderType pass) {
    int i;
    for (i=0; i < objClass->objects.size(); i++) {
        glLoadName(*name);
        glPushName(*name);
        if (lookup) {
            lookup[*name] = (GLuint)((selection * MAX_SCENARIO_OBJECTS) + i);
            (*name)++;
        }
        render_instance(objClass->objects[i], pass);
        glPopName();
    }
}

void ObjectManager::fast_render_subclass(ObjectClass* objClass, SelectionType selection, ShaderType pass) {
    int i;
    for (i=0; i < objClass->objects.size(); i++) {
        render_instance(objClass->objects[i], pass);
    }
}

void clear_selection(ObjectClass* objClass) {
    int i;
    for (i=0; i < objClass->objects.size(); i++) {
        objClass->objects[i]->selected = false;
    }
}

ObjectInstance *ObjectManager::duplicate(ObjectInstance *instance) {
    printf("duplicate object\n");
    SelectionType ctype = instance->type();
    ObjectInstance *copy = instance->duplicate();
    getClass(ctype)->objects.push_back(copy);
    return copy;
}

void ObjectManager::render(GLuint *name, GLuint *lookup, ShaderType pass) {
    if (lookup == nullptr) {
        fast_render_subclass(scen, s_scenery, pass);
        fast_render_subclass(vehi, s_vehicle, pass);
        fast_render_subclass(itmc, s_item, pass);
    } else {
        render_subclass(scen, s_scenery, name, lookup, pass);
        render_subclass(vehi, s_vehicle, name, lookup, pass);
        render_subclass(itmc, s_item, name, lookup, pass);
    }
}

ObjectManager::ObjectManager(Camera *camera, BSP* bsp) {
    this->bsp = bsp;
    this->camera = camera;
}

void ObjectManager::clearSelection() {
    selection.clear();
    clear_selection(scen);
    clear_selection(vehi);
    clear_selection(itmc);
}

void ObjectManager::select(bool shift, float x, float y) {
    printf("select %f %f\n",x,y);
    
    const GLsizei bufferSize = 16384;
    GLuint nameBuf[bufferSize];
    GLuint tmpLookup[bufferSize];
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);
    glSelectBuffer(bufferSize,nameBuf);
    glRenderMode(GL_SELECT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    GLdouble selectWidth = 1.0;
    GLdouble selectHeight = 1.0;
    gluPickMatrix((GLdouble)(x + selectWidth / 2),(GLdouble)(y + selectHeight / 2),(GLdouble)(selectWidth),(GLdouble)(selectHeight),viewport);
    gluPerspective(45.0f,(GLfloat)((GLfloat)(viewport[2] - viewport[0])/(GLfloat)(viewport[3] - viewport[1])),0.1f,400000.0f);
    glMatrixMode(GL_MODELVIEW);

    GLuint name = 1;
    GLuint *lookup = (GLuint *)tmpLookup;
    glInitNames();
    
    shader_options *options = new shader_options;
    
    printf("render names\n");
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnableClientState(GL_VERTEX_ARRAY);
    ShaderType type = static_cast<ShaderType>(shader_SOSO);
    shader *shader = modelManager->shaders->get_shader(shader_SOSO);
    shader->start(options);
    render(&name, lookup, shader_SOSO);
    shader->stop();
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopAttrib();
    free(options);
    printf("done render names\n");
    
    GLuint hits = glRenderMode(GL_RENDER);
    GLuint names, *ptr = (GLuint *)nameBuf;
    if (hits == 0) {
        clearSelection();
        return;
    }

    int i;
    unsigned int  j, z1, z2;
    for (i = 0; i < hits; i++) {
        names = *ptr;
        ptr++;
        z1 = (float)*ptr/0x7fffffff;
        ptr++;
        z2 = (float)*ptr/0x7fffffff;
        ptr++;
        for (j = 0; j < names; j++) {
            if (*ptr >= bufferSize)
                break;
            printf("%d %d\n", bufferSize, *ptr);
            int type  = (unsigned int)(lookup[*ptr] / MAX_SCENARIO_OBJECTS);
            int index = (unsigned int)(lookup[*ptr] % MAX_SCENARIO_OBJECTS);
            
            SelectionType ctype = static_cast<SelectionType>(type);
            ObjectInstance *instance = getClass(ctype)->objects[index];
            if (!instance->selected) {
                instance->selected = true;
                selection.push_back(instance);
            }
        }
    }
    
    // If shift is down, duplicate all of the selected objects
    if (shift) {
        std::vector<ObjectInstance*> duplicates;
        duplicates.resize(selection.size());
        int i;
        for (i=0; i < selection.size(); i++) {
            duplicates[i] = duplicate(selection[i]);
        }
        clearSelection();
        selection.resize(duplicates.size());
        for (i=0; i < selection.size(); i++) {
            selection[i] = duplicates[i];
            selection[i]->selected = true;
        }
    }
    glPopMatrix();
}