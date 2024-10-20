#include "Mode.hpp"

#include "Connection.hpp"
#include "Game.hpp"
#include "Scene.hpp"
#include "WalkMesh.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----


	//input tracking for local player:
	Player::Controls controls;

	//latest game state (from server):
	Game game;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;

	//GAME CODE //

	struct Animation{
	std::vector<std::vector<Scene::Transform*>> frames; //a vector of vectors
	std::vector<std::vector<glm::vec3>> scales; //a vector of scales
	int fps = 12;
	int numframes;
	int currframe = 0;
	}duckrun, duckidle;
	
	float playeranimtimer = 0.0f;

	glm::highp_quat *animrot;

	std::vector<Animation> animations;

	//----- game state -----

	//local copy of the game scene (so code can change it during gameplay):
	Scene mainscene;
	Scene objscene;
	
	glm::highp_quat rotatecam; //set camera rotate 
	glm::highp_quat origcamrot; //past camera rotate
	Scene::Transform *playertranslate; //translation of player
	Scene::Transform *camrotate; //rotation of camera
	Scene::Transform *camtranslate; //translation of camera
	Scene::Transform *bbox; //bounding box of player for collision detection
	glm::highp_quat duckheadrotate; //rotation of player head
	glm::vec3 playerhead; //position of the head of the player

	float rotateholdx;
	bool duckrotated = false;
	glm::highp_quat pastrotation;
	
	
	int animtype;
	int animframe;

	
	std::vector<Scene::Transform*> wallpos;

	int score = 0;


	//player info:
	struct Player {
		WalkPoint at;
		//transform is at player's feet and will be yawed by mouse left/right motion:
		Scene::Transform *transform = nullptr;
		//camera is at player's head and will be pitched by mouse up/down motion:
		Scene::Camera *camera = nullptr;
	} player;

};
