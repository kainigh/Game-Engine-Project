
// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <list>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include "SphereMesh.h"
#include "Texture.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

class Transform{

    protected:
        //Local space
       glm::vec3 m_pos = {0.0f, 0.0f, 0.0f};
       glm::vec3 m_eulerRot = {0.0f, 0.0f, 0.0f};
       glm::vec3 m_scale = {1.0f, 1.0f, 1.0f};

        //Global space
       glm::mat4 m_modelMatrix = glm::mat4(1.0f);

       bool m_isDirty = true;

    protected:  
       glm::mat4 getLocalModelMatrix()
       {
            const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(m_eulerRot.x)), glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(m_eulerRot.y)), glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(m_eulerRot.z)), glm::vec3(0.0f, 0.0f, 1.0f));

            // Y * X * Z
            const glm::mat4 roationMatrix = transformY * transformX * transformZ;

            // translation * rotation * scale (also know as TRS matrix)
            return glm::translate(glm::mat4(1.0f), m_pos) * roationMatrix * glm::scale(glm::mat4(1.0f), m_scale);
       }

    public:
       
        void computeModelMatrix()
        {
            m_modelMatrix = getLocalModelMatrix();
            m_isDirty = false;
        }

        void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
        {
            m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
            m_isDirty = false;
        }

       void setLocalPosition(const glm::vec3& newPosition)
       {
            m_pos = newPosition;
            m_isDirty = true;
       }

       void setLocalScale(const glm::vec3& newScale)
       {
            m_scale = newScale;
            m_isDirty = true;
       }

       void setLocalRotation(const glm::vec3& newRotation)
       {
            m_eulerRot = newRotation;
            m_isDirty = true;
       }

       const glm::vec3& getLocalRotation() const
       {
            return m_eulerRot;
       }

       const glm::vec3& getLocalPosition() const
	   {
		    return m_pos;
	   }
           
        const glm::vec3& getLocalScale() const
        {   
            return m_scale;
        }
        
        const glm::mat4& getModelMatrix() const
	    {
		    return m_modelMatrix;
	    }

        bool isDirty() const
        {
            return m_isDirty;
        }


};


class Entity{

    public:
        std::list<std::unique_ptr<Entity>> children;

	    Entity* parent = nullptr;
        Transform m_transform;
        glm::mat4 modelMatrix;
        GLuint textureID;  // Add this line inside the Entity class
        SphereMesh* mesh = nullptr;
        Textures* texture = nullptr;  // NEW



        //Entity(SphereMesh* sharedMesh) : mesh(sharedMesh) {}
        Entity(SphereMesh* mesh, Textures* texture) : mesh(mesh), texture(texture) {}

        Entity(SphereMesh* mesh) : mesh(mesh) {}

        template<typename... TArgs>
        void addChild(TArgs&... args)
        {
            children.emplace_back(std::make_unique<Entity>(std::forward<TArgs>(args)...));
            children.back()->parent = this;
        }

        // Version B: accept a prebuilt entity
        void addChild(std::unique_ptr<Entity> child)
        {
            child->parent = this;
            children.emplace_back(std::move(child));
        }

        void updateSelfAndChild()
	    {
            if (m_transform.isDirty()) {
                forceUpdateSelfAndChild();
                return;
            }
                
            for (auto&& child : children)
            {
                child->updateSelfAndChild();
            }
	    }

        //Force update of transform even if local space don't change
        void forceUpdateSelfAndChild()
        {
            if (parent)
                m_transform.computeModelMatrix(parent->m_transform.getModelMatrix());
            else
                m_transform.computeModelMatrix();

            for (auto&& child : children)
            {
                child->forceUpdateSelfAndChild();
            }
        }

        
};