#include "VertexArray.h"
#include "Window.h"
#include "VertexBufferLayout.h"

VertexArray::VertexArray() {
	glGenVertexArrays(1, &m_RendererID);
};

VertexArray::~VertexArray() {
	GLCall(glDeleteVertexArrays(1, &m_RendererID));
};

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) {
	Bind();
	vb.Bind();
	const auto& elements = layout.GetElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size();i++) {
		const auto& element = elements[i];
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset);
		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
	}
	
}


 void VertexArray::Bind() const {
	glBindVertexArray(m_RendererID);
} ;
void VertexArray::Unbind() const{
	glBindVertexArray(0);
};

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout, int instanceDivisor) {
    Bind();
    vb.Bind();
    
    const auto& elements = layout.GetElements();
    unsigned int offset = 0;
    
    // Query how many attributes are already enabled to get the starting index
    GLint maxAttribs;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    
    unsigned int startIndex = 0;
    for (unsigned int i = 0; i < (unsigned int)maxAttribs; i++) {
        GLint enabled;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        if (enabled) {
            startIndex = i + 1;
        } else {
            break;
        }
    }
    
    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto& element = elements[i];
        unsigned int index = startIndex + i;
        
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(
            index, 
            element.count, 
            element.type, 
            element.normalized ? GL_TRUE : GL_FALSE,
            layout.GetStride(), 
            (const void*)(uintptr_t)offset
        );
        
        // Set divisor for instanced rendering
        // 0 = per-vertex data (default)
        // 1 = per-instance data (advances once per instance)
        glVertexAttribDivisor(index, instanceDivisor);
        
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
    }
}

