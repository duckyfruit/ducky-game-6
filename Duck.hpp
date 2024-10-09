#include "Mode.hpp"

#include "Scene.hpp"
#include "WalkMesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>


struct Duck{

    Duck();

    //Scene scene; //each duck should have a separate scene where it can grab its own meshes from to
                 //render in

    void move(uint8_t up, uint8_t down, uint8_t left, uint8_t right, float elapsed); //move the duck 
    
    void setTransforms(); //set the transforms of the duck when created

    void setAnimations(); //set the animations of the duck when created
    

	struct Animation{
        std::vector<std::vector<Scene::Transform*>> frames; //a vector of vectors
        std::vector<std::vector<glm::vec3>> scales; //a vector of scales
        int fps = 12;
        int numframes;
        int currframe = 0;
	}duckrun, duckidle;
    
    float playeranimtimer = 0.0f;
    bool duckrotated = false;
	
    struct transforms{ //the transforms of the duck belonging to the player
        glm::highp_quat pastrotation; //the past camera rotation

        glm::highp_quat *animrot = nullptr; //rotate the mesh itself
        
        Scene::Transform *playertranslate = nullptr; //the base duck translate
                        //camrotate --> player translate <--- player.transform

        float rotateholdx = 0.0f; //the rotation parameter 

        bool ismoving = false; //bool for switching animations
    }ducktransform;
	
	
	struct Player {//player info: walkpoint, transform walkpoint and camera 
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} duck;
};