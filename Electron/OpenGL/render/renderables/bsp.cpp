//
//  render.h
//  
//
//  Created by Samuco on 19/04/2015.
//
//

#include "bsp.h"

uint8_t* map2mem(ProtonTag *scenario, uint32_t address) {
    return (uint8_t*)(scenario->Data() + scenario->PointerToOffset(address));
}

void BSPRenderBuffer::setup() {
    // Create the buffers for the vertices atttributes
    glGenVertexArraysAPPLE(1, &geometryVAO);
    glBindVertexArrayAPPLE(geometryVAO);
    
    // Create the buffers for the vertices atttributes
    glGenBuffers(5, m_Buffers);
}

void BSPRenderMesh::setup() {
    // Create the buffers for the vertices atttributes
    glGenVertexArraysAPPLE(1, &geometryVAO);
    glBindVertexArrayAPPLE(geometryVAO);
    
    // Create the buffers for the vertices atttributes
    glGenBuffers(5, m_Buffers);
}

BSP::BSP(ShaderManager* manager) {
    printf("bsp setup\n");
    shaders = manager;
}

//TODO: Group elements into renderables
void BSP::setup(ProtonMap *map, ProtonTag *scenario) {
    
    vao = new BSPRenderBuffer;
    
    // Count data size
    int index_size  = 0;
    int vertex_size = 0;
    int renderable_count = 0;
    HaloTagReflexive bsp = ((HaloScenarioTag*)scenario->Data())->bsp;
    int i, m, n;
    for (i=0; i < bsp.count; i++) {
        BSP_CHUNK *chunk = (BSP_CHUNK *)(map2mem(scenario, bsp.address) + sizeof(BSP_CHUNK) * i); // VERIFIED
        ProtonTag *bspTag = map->tags.at((uint16_t)(chunk->tagId)).get();
        uint32_t mesh_offset = *(uint32_t *)(bspTag->Data());
        BSP_MESH *mesh = (BSP_MESH *)map2mem(bspTag, mesh_offset);
        for (m=0; m < mesh->submeshHeader.count; m++) {
            BSP_SUBMESH *submesh = (BSP_SUBMESH *)(map2mem(bspTag, mesh->submeshHeader.address) + sizeof(BSP_SUBMESH) * m);
            for (n=0; n < submesh->material.count; n++) {
                MATERIAL_SUBMESH_HEADER *material = (MATERIAL_SUBMESH_HEADER *)(map2mem(bspTag, submesh->material.address) + sizeof(MATERIAL_SUBMESH_HEADER) * n);
                vertex_size += material->VertexCount1;
                index_size += material->VertIndexCount * 3;
                renderable_count++;
            }
        }
    }
    renderables.resize(renderable_count);
    
    vao->setup();
    vao->vertex_array    = (GLfloat*)malloc(vertex_size   * 3 * sizeof(GLfloat));
    vao->texture_uv      = (GLfloat*)malloc(vertex_size   * 2 * sizeof(GLfloat));
    vao->light_uv        = (GLfloat*)malloc(vertex_size   * 2 * sizeof(GLfloat));
    vao->normals         = (GLfloat*)malloc(vertex_size   * 3 * sizeof(GLfloat));
    vao->index_array     =   (GLint*)malloc(index_size    *     sizeof(GLint));

    int vertex_offset = 0;
    int index_offset = 0;
    int render_pos = 0;
    
    int vert, uv;
    for (i=0; i < bsp.count; i++) {
        BSP_CHUNK *chunk = (BSP_CHUNK *)(map2mem(scenario, bsp.address) + sizeof(BSP_CHUNK) * i); // VERIFIED
        ProtonTag *bspTag = map->tags.at((uint16_t)(chunk->tagId)).get();
        uint32_t mesh_offset = *(uint32_t *)(bspTag->Data());
        BSP_MESH *mesh = (BSP_MESH *)map2mem(bspTag, mesh_offset);
        for (m=0; m < mesh->submeshHeader.count; m++) {
            BSP_SUBMESH *submesh = (BSP_SUBMESH *)(map2mem(bspTag, mesh->submeshHeader.address) + sizeof(BSP_SUBMESH) * m);
            for (n=0; n < submesh->material.count; n++) {
                MATERIAL_SUBMESH_HEADER *material = (MATERIAL_SUBMESH_HEADER *)(map2mem(bspTag, submesh->material.address) + sizeof(MATERIAL_SUBMESH_HEADER) * n);
                uint8_t *vertIndexOffset = (uint8_t *)((sizeof(TRI_INDICES) * material->VertIndexOffset) + map2mem(bspTag, mesh->submeshIndices.address));
                uint8_t *PcVertexDataOffset = map2mem(bspTag, material->PcVertexDataOffset);
                int vertex_number = material->VertexCount1;
                int indexSize = material->VertIndexCount*3;
                
                HaloTagDependency shader = material->ShaderTag;
                printf("shader setup %d %d\n", n, shader.tag_id.tag_index);
                shader_object *material_shader = shaders->create_shader(map, shader);
                
                printf("renderer setup %d\n", n);
                BSPRenderMesh *renderer = new BSPRenderMesh;
                renderer->shader = material_shader;
                renderer->indexOffset = index_offset;
                renderer->vertexOffset = vertex_offset;
                renderer->indexCount = indexSize;
                renderer->vertCount = vertex_number;
                
                int v;
                for (v = 0; v < vertex_number; v++)
                {
                    UNCOMPRESSED_BSP_VERT *vert1 = (UNCOMPRESSED_BSP_VERT*)(PcVertexDataOffset + v * sizeof(UNCOMPRESSED_BSP_VERT));
                    vert = vertex_offset * 3;
                    uv = vertex_offset * 2;
                    vao->vertex_array[vert]   = vert1->vertex_k[0];
                    vao->vertex_array[vert+1] = vert1->vertex_k[1];
                    vao->vertex_array[vert+2] = vert1->vertex_k[2];
                    vao->normals[vert]        = vert1->normal[0];
                    vao->normals[vert+1]      = vert1->normal[1];
                    vao->normals[vert+2]      = vert1->normal[2];
                    vao->texture_uv[uv]       = vert1->uv[0];
                    vao->texture_uv[uv+1]     = vert1->uv[1];
                    vertex_offset++;
                }
                
                for (v = 0; v < indexSize; v+=3)
                {
                    TRI_INDICES *index = (TRI_INDICES*)(vertIndexOffset + sizeof(uint16_t) * v);
                    vao->index_array[index_offset]   = index->tri_ind[0];
                    vao->index_array[index_offset+1] = index->tri_ind[1];
                    vao->index_array[index_offset+2] = index->tri_ind[2];
                    index_offset += 3;
                }
                
                renderables[render_pos] = renderer;
                render_pos++;
            }
        }
    }
    
    // Assemble the VAO
    #define texCoord_buffer 1
    #define normals_buffer 2
    #define texCoord_buffer_light 3
        
    //Shift these to vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, vao->m_Buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size * 3 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
    GLvoid* my_vertex_pointer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(my_vertex_pointer, vao->vertex_array, vertex_size * 3 * sizeof(GLfloat));
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, vao->m_Buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size * 2 * sizeof(GLfloat), vao->texture_uv, GL_STATIC_DRAW);
    glEnableVertexAttribArray(texCoord_buffer);
    glVertexAttribPointer(texCoord_buffer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, vao->m_Buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size * 3 * sizeof(GLfloat), vao->normals, GL_STATIC_DRAW);
    glEnableVertexAttribArray(normals_buffer);
    glVertexAttribPointer(normals_buffer, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size * sizeof(GLint), vao->index_array, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, vao->m_Buffers[LIGHT_VB]);
    glBufferData(GL_ARRAY_BUFFER, vertex_size * 2 * sizeof(GLfloat), vao->light_uv, GL_STATIC_DRAW);
    glEnableVertexAttribArray(texCoord_buffer_light);
    glVertexAttribPointer(texCoord_buffer_light, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindVertexArrayAPPLE(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void BSP::render(ShaderType pass) {
    
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
#ifndef RENDER_VAO
    glEnableVertexAttribArray(texCoord_buffer);
    glEnableVertexAttribArray(normals_buffer);
#endif
    
#ifdef RENDER_VAO
    glBindVertexArrayAPPLE(vao->geometryVAO);
#else
    glBindVertexArrayAPPLE(0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->m_Buffers[POS_VB]);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->m_Buffers[TEXCOORD_VB]);
    glVertexAttribPointer(texCoord_buffer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->m_Buffers[NORMAL_VB]);
    glVertexAttribPointer(normals_buffer, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->m_Buffers[INDEX_BUFFER]);
#endif
    
    int i;
    shader_object *previous_shader = nullptr;
    for (i=0; i < renderables.size(); i++) {
        BSPRenderMesh *mesh = renderables[i];
        if (mesh->shader != nullptr && mesh->shader->is(pass)) {
            if (mesh->shader != previous_shader) {
                mesh->shader->render();
                previous_shader = mesh->shader;
            }
            glDrawElementsBaseVertex(GL_TRIANGLES,
                                     mesh->indexCount,
                                     GL_UNSIGNED_INT,
                                     (void*)(mesh->indexOffset * sizeof(GLuint)),
                                     (mesh->vertexOffset));
        }
    }
    glBindVertexArrayAPPLE(0);
}