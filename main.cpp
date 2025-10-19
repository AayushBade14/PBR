#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "./stb_image.h"

int WIDTH = 1920;
int HEIGHT = 1013;
const char* TITLE = "PBR-DEMO";

class Shader{
private:
  unsigned int mId;

  std::string LoadFile(const std::string& path){
    std::string code;
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
      file.open(path);
      std::stringstream stream;
      stream << file.rdbuf();
      file.close();
      code = stream.str();
    }
    catch(const std::ifstream::failure& e){
      std::cerr<<"ERROR: "<<e.what()<<std::endl;
      exit(1);
    }
    return code;
  }

  unsigned int CompileShader(const std::string& srcCode, bool isVert){
    const char* code = srcCode.c_str();
    int success;
    char infoLog[512];
    unsigned int shader = isVert? glCreateShader(GL_VERTEX_SHADER) : glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success){
      glGetShaderInfoLog(shader, 512, nullptr, infoLog);
      std::cerr<<"ERROR: compiling "<<(isVert?" vertex":" fragment")<<" shader -> "<<infoLog<<std::endl;
      exit(1);
    }
    return shader;
  }

  void CreateShaderProgram(unsigned int& vert, unsigned int& frag){
    mId = glCreateProgram();
    glAttachShader(mId, vert);
    glAttachShader(mId, frag);
    glLinkProgram(mId);

    int success;
    char infoLog[512];

    glGetProgramiv(mId, GL_LINK_STATUS, &success);
    if(!success){
      glGetProgramInfoLog(mId, 512, nullptr, infoLog);
      std::cerr<<"ERROR: linking shader program -> "<<infoLog<<std::endl;
      exit(1);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    std::cout<<"Successfully created shader program!"<<std::endl;
  }

public:
  Shader(const std::string& vertPath, const std::string& fragPath){
    std::string vCode = LoadFile(vertPath);
    std::string fCode = LoadFile(fragPath);
    unsigned int vert = CompileShader(vCode, true);
    unsigned int frag = CompileShader(fCode, false);
    CreateShaderProgram(vert, frag);
  }

  ~Shader() {glDeleteProgram(mId);}

  void Use() {glUseProgram(mId);}

  template <typename T>
  void SetValue(const std::string& name, const T& val){
    const unsigned int loc = glGetUniformLocation(mId, name.c_str());

    if constexpr(std::is_same_v<T,int>) glUniform1i(mId, val);
    else if constexpr(std::is_same_v<T,bool>) glUniform1i(mId, (int)val);
    else if constexpr(std::is_same_v<T,float>) glUniform1f(mId, val);
    else if constexpr(std::is_same_v<T,glm::vec2>) glUniform2fv(mId, 1, glm::value_ptr(val));
    else if constexpr(std::is_same_v<T,glm::vec3>) glUniform3fv(mId, 1, glm::value_ptr(val));
    else if constexpr(std::is_same_v<T,glm::mat4>) glUniformMatrix4fv(mId, 1, GL_FALSE, glm::value_ptr(val));
  }
};

class VBO{
private:
  unsigned int mId;

public:
  VBO(){glGenBuffers(1, &mId);}
  ~VBO(){glDeleteBuffers(1, &mId);}

  VBO(const VBO&) = delete;
  VBO(VBO&& other) noexcept : mId(other.mId){other.mId = 0;}
  VBO& operator=(VBO&& other) noexcept{
    if(this != other){
      glDeleteBuffers(1, &mId);
      mId = other.mId;
      other.mId = 0;
    }
    return *this;
  }

  void Bind(){glBindBuffer(GL_ARRAY_BUFFER, mId);}
  void Unbind(){glBindBuffer(GL_ARRAY_BUFFER, mId);}

  void AllocateMem(size_t size, GLenum usage){glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);}
  void FillMem(size_t offset, size_t size, const void* data, GLenum usage){glBufferSubData(GL_ARRAY_BUFFER, offset, size, data, usage);}
  void AllocateAndFillMem(size_t size, const void* data, GLenum usage){glBufferData(GL_ARRAY_BUFFER, size, data, usage);}
};

class EBO{
private:
  unsigned int mId;

public:
  EBO(){glGenBuffers(1, &mId);}
  ~EBO(){glDeleteBuffers(1, &mId);}

  EBO(const EBO&) = delete;
  EBO(EBO&& other) noexcept : mId(other.mId){other.mId = 0;}
  EBO& operator=(EBO&& other) noexcept{
    if(this != other){
      glDeleteBuffers(1, &mId);
      mId = other.mId;
      other.mId = 0;
    }
    return *this;
  }

  void Bind(){glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mId);}
  void Unbind(){glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mId);}

  void AllocateMem(size_t size, GLenum usage){glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, usage);}
  void FillMem(size_t offset, size_t size, const void* data, GLenum usage){glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data, usage);}
  void AllocateAndFillMem(size_t size, const void* data, GLenum usage){glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);}
};

class VAO{
private:
  unsigned int mId;

public:
  VAO(){glGenVertexArrays(1, &mId);}
  ~VAO(){glDeleteVertexArrays(1, &mId);}

  VAO(const VAO&) = delete;
  VAO(VAO&& other) noexcept : mId(other.mId){other.mId = 0;}
  VAO& operator=(VAO&& other) noexcept{
    if(this != other){
      glDeleteVertexArrays(1, &mId);
      mId = other.mId;
      other.mId = 0;
    }
    return *this;
  }

  void Bind(){glBindVertexArrays(mId);}
  void Unbind(){glBindVertexArrays(0);}

  void SetAttrib(int loc, int nr, size_t stride, size_t offset){
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, nr, GL_FLOAT, GL_FALSE, stride, (void*)offset);
  }
};

struct Vertex{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texcoord;
};

struct Texture{
  std::string type;
  unsigned int id;
};

class Mesh{
private:  
  VBO mVbo;
  EBO mEbo;
  VAO mVao;

  void SetupMesh(){
    mVao.Bind();
    mVbo.Bind();
    mVbo.AllocateAndFillMem(mVertices.size() * sizeof(Vertex), mVertices.data(), GL_STATIC_DRAW);
    mEbo.Bind();
    mEbo.AllocateAndFillMem(mIndices.size() * sizeof(unsigned int), mIndices.data(), GL_STATIC_DRAW);
    mVao.SetAttrib(0, 3, sizeof(Vertex), offsetof(Vertex, position));
    mVao.SetAttrib(1, 3, sizeof(Vertex), offsetof(Vertex, normal));
    mVao.SetAttrib(2, 2, sizeof(Vertex), offsetof(Vertex, texcoord));

    mVao.Unbind();
  }

public:
  std::vector<Vertex> mVertices;
  std::vector<unsigned int> mIndices;
  std::vector<Texture> mTextures;
  
  Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures){
    mVertices = vertices;
    mIndices = indices;
    mTextures = textures;
    SetupMesh();
  }

  ~Mesh()=default;

  void Draw(Shader& shader){
    shader.Use();

    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    for(unsigned int i = 0; i < mTextures.size(); i++){
      glActiveTexture(GL_TEXTURE0 + i);

      std::string number;
      std::string name = mTextures[i].type;
      if(name == "texture_diffuse") number = std::to_string(diffuseNr++);
      else if(name == "texture_specular") number = std::to_string(specularNr++);
      
      shader.SetValue("material."+name+number, i);
      glBindTexture(GL_TEXTURE_2D, mTextures[i].id);
    }
    glActiveTexture(GL_TEXTURE0);

    mVao.Bind();
    glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0);
    mVao.Unbind();
  }
};
