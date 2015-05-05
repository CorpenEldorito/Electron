//
//  shader.h
//  
//
//  Created by Samuco on 19/04/2015.
//
//

#ifndef ____EShaderSoso__
#define ____EShaderSoso__

#include "../shader.h"
class soso : public shader {
private:
    GLuint program;
    GLint baseTexture;
    GLint multipurposeMap;
    GLint detailMap;
    GLint cubeMap;
public:
    GLint scale;
    GLint maps;
    GLint reflectionScale;
    
    void setup(std::string path);
    void start();
    void stop();
};

class soso_object : public shader_object {
private:
    float uscale = 1.0;
    float vscale = 1.0;
    texture *baseMap = nullptr;
    
    bool useMulti = false;
    texture *multipurposeMap = nullptr;
    
    bool useDetail = false;
    float detailScale = 1.0;
    float detailScaleV = 1.0;
    texture *detailMap = nullptr;
    
    bool useCube = false;
    texture_cubemap *cubeMap = nullptr;
    float reflectionPerpendicular;
    float reflectionParallel;
    
    GLint mapsId;
    GLint scaleId;
    GLint reflectionScaleId;
public:
    void setup(ShaderManager *manager, ProtonMap *map, ProtonTag *shaderTag);
    bool render();
    void setBaseUV(float u, float v);
    bool is(ShaderType type);
};

#endif
