//
// Created by aidoo on 2024/11/10.
//

#include <string>

#ifndef _SHADERS_H
#define _SHADERS_H

#define GLSL(src) "precision mediump float;\n" #src


const std::string default_vertex_shader =
        GLSL(
                attribute vec4 a_position;
                attribute vec2 a_texCoord;
                varying vec2 v_texCoord;
                uniform bool u_flipVertical;

                void main() {
                    gl_Position = a_position;
                    if (u_flipVertical) {
                        v_texCoord = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
                    } else {
                        v_texCoord = a_texCoord;
                    }
                }

        );


const std::string default_fragment_shader =
        GLSL(
                uniform sampler2D u_texture;
                varying vec2 v_texCoord;

                void main() {
                    gl_FragColor = texture2D(u_texture, v_texCoord);
                }

        );


#endif
