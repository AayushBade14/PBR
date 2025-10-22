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

GLFWwindow* window = nullptr;

float dt = 0.0f;
float lastFrame = 0.0f;


bool IsKeyPressed(GLenum key){
  return glfwGetKey(window, key)==GLFW_PRESS;
}

bool IsKeyReleased(GLenum key){
  return glfwGetKey(window, key)==GLFW_RELEASE;
}

bool IsMousePressed(GLenum button){
  return glfwGetMouseButton(window, button)==GLFW_PRESS;
}

bool IsMouseReleased(GLenum button){
  return glfwGetMouseButton(window, button)==GLFW_RELEASE;
}

void ProcessInput(){
  if(IsKeyPressed(GLFW_KEY_ESCAPE))
    glfwSetWindowShouldClose(window, true);

  if(IsKeyPressed(GLFW_KEY_T))
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  if(IsKeyPressed(GLFW_KEY_Y))
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void UpdateTimer(){
  float currentFrame = (float)glfwGetTime();
  dt = currentFrame - lastFrame;
  lastFrame = currentFrame;
}

void UpdateWindow(){
  double x;
  double y;

  glfwGetWindowSize(window, &x, &y);

  WIDTH = x;
  HEIGHT = y;
}

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
  std::string path;
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

class Model{
private:
  std::vector<Mesh> mModelMeshes;
  std::string directory;

  void ProcessNode(aiNode* node, const aiScene* scene){
    for(unsigned int i = 0; i < node->mNumMeshes; i++){
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      mModelMeshes.push_back(ProcessMesh(mesh, scene));
    }

    for(unsigned int i = 0; i < node->mNumChildren; i++){
      ProcessNode(node->mChildren[i], scene);
    }
  }

  Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene){
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for(unsigned int i = 0; i < mesh->mNumVertices; i++){
      Vertex v;
      v.position.x = mesh->mVertices[i].x;
      v.position.y = mesh->mVertices[i].y;
      v.position.z = mesh->mVertices[i].z;

      if(mesh->HasNormals()){
        v.normal.x = mesh->mNormals[i].x;
        v.normal.y = mesh->mNormals[i].y;
        v.normal.z = mesh->mNormals[i].z;
      }
      else{
        v.normal = {0.0f,0.0f,1.0f};
      }

      if(mesh->mTextureCoords[0]){
        v.texcoord.x = mTextureCoords[0][i].x;
        v.texcoord.y = mTextureCoords[0][i].y;
      }
      else{
        v.texcoord = {0.0f,0.0f};
      }

      vertices.push_back(v);
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++){
      aiFace face = mesh->mFaces[i];
      for(unsigned int j = 0; j < face.mNumIndices; i++){
        indices.push_back(face.mIndices[i]);
      }
    }
    
    // check if that mesh has an assigned material
    if(mesh->mMaterialIndex >= 0){
      aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
      std::vector<Texture> diffuseMaps = LoadMaterialTexture(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
      std::vector<Texture> specularMaps = LoadMaterialTexture(material, aiTextureType_SPECULAR, "texture_specular", scene);
      std::vector<Texture> normalMaps = LoadMaterialTexture(material, aiTextureType_HEIGHT, "texture_normal", scene);
      textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
      textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
      textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    }

    return Mesh(vertices, indices, textures);
  }
  
  std::vector<Texture> LoadMaterialTexture(aiMaterial* mat, aiTextureType type, const std::string& typeName, const aiScene* scene){
    std::vector<Texture> textures;
    
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++){
      aiString str;
      mat->GetTexture(type, i, &str);

      Texture texture;

      if(str.C_Str()[0] == "*"){
        int index = atoi(str.C_Str() + 1);
        const aiTexture* tex = scene->mTextures[index];
        texture.id = (tex->mHeight == 0)? TextureFromMemoryCompressed(tex->pcData, tex->mWidth) : TextureFromMemory(tex->pcData, tex->mWidth, tex->mHeight);
      }
      else{
        std::string filename = std::string(str.C_Str());
        filename = directory + "/" + filename;
        texture.id = TextureFromFile(filename);
      }

      texture.type = typeName;
      texture.path = str.C_str();
      textures.push_back(texture);
    }
    return textures;
  }

  unsigned int TextureFromMemoryCompressed(const aiTexel* pixels, int mwidth){
    unsigned int texid;
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
    
    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(pixels), mwidth, &width, &height, &nrChannels, 0);
    
    if(data){
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return texid;
  }

  unsigned int TextureFromMemory(const aiTexel* pixels, int mwidth, int mheight){
    unsigned int texid;
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mwidth, mheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     
    return texid;
  }

  unsigned int TextureFromFile(const std::string& path){
    unsigned int texid;
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);

    int width;
    int height;
    int nrChannels;
    
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if(data){
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D); 
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(data);

    return texid;
  }

public:
  Model() {}

  Model(const std::string& path){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_GenNormals);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
      std::cerr<<"ASSIMP::ERROR: "<<importer.GetErrorString()<<std::endl;
      exit(1);
    }
    
    ProcessNode(scene->mRootNode, scene);
  }

  ~Model()=default;

  void Draw(Shader& shader){
    shader.Use();
    for(auto& mesh: mModelMeshes)
      mesh.Draw(shader);
  }

  // Add custom binary format for model loading/saving
};

class Camera{
private:
  glm::vec3 mPosition;
  glm::vec3 mFront;
  glm::vec3 mUp;

  float mYaw;
  float mPitch;
  float mFov;

  float mSpeed;
  float mSensitivity;

  float mLastX;
  float mLastY;

  bool mFirstMouse;
  
  glm::mat4 mViewMatrix;
  glm::mat4 mProjectionMatrix;
  
  void UpdateMovement(float dt){
    if(IsKeyPressed(GLFW_KEY_W)) mPosition += mSpeed * dt * mFront;
    if(IsKeyPressed(GLFW_KEY_S)) mPosition -= mSpeed * dt * mFront;
    if(IsKeyPressed(GLFW_KEY_A)) mPosition -= mSpeed * dt * glm::normalize(glm::cross(mFront, mUp));
    if(IsKeyPressed(GLFW_KEY_D)) mPosition += mSpeed * dt * glm::normalize(glm::cross(mFront, mUp));
    if(IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) mSpeed = 10.0f;
    else mSpeed = 6.0f;
  }

public:
  Camera(float speed, float sensitivity): mSpeed(speed), mSensitivity(sensitivity){
    mPosition = {0.0f, 0.0f, 3.0f};
    mFront = {0.0f, 0.0f, -1.0f};
    mUp = {0.0f, 1.0f, 0.0f};

    mYaw = -90.0f;
    mPitch = 0.0f;
    mFov = 45.0f;

    mLastX = 0.0f;
    mLastY = 0.0f;

    mFirstMouse = true;
    
    mViewMatrix = glm::mat4(1.0f);
    mProjectionMatrix = glm::mat4(1.0f);
  }

  ~Camera()=default;
  
  const glm::mat4& GetPosition() const {return mPosition;}
  const glm::mat4& GetFront() const {return mFront;}
  const glm::mat4& GetViewMatrix() const {return mViewMatrix;}
  const glm::mat4& GetProjectionMatrix() const {return mProjectionMatrix;}

  void SetPosition(const glm::vec3& position) {mPosition = position;}
  void SetFront(const glm::vec3& front) {mFront = front;}

  void Update(float dt){
    mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
    mProjectionMatrix = glm::perspective(glm::radians(mFov), (float)WIDTH/(float)HEIGHT, 0.1f, 1000.0f);
    UpdateMovement(dt);
  }

  void UpdateMouse(float x, float y){
    if(mFirstMouse){
      mLastX = x;
      mLastY = y;
      mFirstMouse = false;
    }

    float xoffset = x - mLastX;
    float yoffset = mLastY - y;

    xoffset *= mSensitivity;
    yoffset *= mSensitivity;

    mLastX = x;
    mLastY = y;

    mYaw += xoffset;
    mPitch += yoffset;

    if(mPitch > 89.0f) mPitch = 89.0f;
    if(mPitch < -89.0f) mPitch = -89.0f;

    glm::vec3 direction;
    direction.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
    direction.y = glm::sin(glm::radians(mPitch));
    direction.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));

    mFront = glm::normalize(direction);
  }

  void UpdateScroll(float xoffset, float yoffset){
    mFov -= yoffset;

    if(mFov < 1.0f) mFov = 1.0f;
    if(mFov > 45.0f) mFov = 45.0f;
  }
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
  glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
  Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
  if(camera){
    camera->UpdateMouse(xpos, ypos);
  }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
  Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
  if(camera){
    camera->UpdateScroll(xoffset, yoffset);
  }
}

int main(int argc, char* argv[]){
  
  if(glfwInit() < 0){
    std::cerr<<"ERROR: GLFW::Init()!"<<std::endl;
    exit(1);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(WIDTH,
                            HEIGHT,
                            TITLE,
                            nullptr,
                            nullptr);

  if(!window){
    std::cerr<<"ERROR: Window::Init()!"<<std::endl;
    glfwTerminate();
    exit(1);
  }

  glfwMakeContextCurrent(window);

  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
    std::cerr<<"ERROR: GLAD::Init()!"<<std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(1);
  }

  Shader shader("../vert.glsl", "../frag.glsl");
  Camera camera(6.0f, 0.1f);
  glfwSetWindowUserPointer(window, &camera);
  
  Model monkey("../monkey.obj");
  
  glEnable(GL_DEPTH_TEST);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  
  while(!glfwWindowShouldClose(window)){
    glfwPollEvents();

    UpdateTimer();
    UpdateWindow();

    ProcessInput();
    
    camera.Update(dt);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = camera.GetProjectionMatrix();
    
    shader.Use();
    shader.SetValue("model", model);
    shader.SetValue("view", view);
    shader.SetValue("projection", projection);
    monkey.Draw(shader);

    glfwSwapBuffers(window);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
