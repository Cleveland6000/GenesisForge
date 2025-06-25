#ifndef TEXT_RENDERER_HPP
#define TEXT_RENDERER_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include "FontLoader.hpp"

class TextRenderer
{
public:
    TextRenderer();
    ~TextRenderer();
    bool initialize(const std::string &vertPath, const std::string &fragPath, const FontData &fontData);
    void renderText(const std::string &text, float x, float y, float scale, const glm::vec3 &color, const glm::mat4 &projection);

private:
    GLuint m_textVAO, m_textVBO, m_textShaderProgram;
    const FontData *m_fontData;
};

#endif // TEXT_RENDERER_HPP
