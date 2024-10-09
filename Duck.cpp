#include "Duck.hpp"
#include "data_path.hpp"
#include "Load.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <random>
#include <iostream>
#include <stdexcept>

WalkMesh const *playerwalkmesh = nullptr;

Scene playerscene;


Duck::Duck()
{
	

	Scene*ret = new Scene(data_path("duckyanimations.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){});
	playerscene = *ret;

	WalkMeshes *walk = new WalkMeshes(data_path("duckyanimations.w"));
	playerwalkmesh = &walk->lookup("WalkMesh");

	ducktransform.rotateholdx = 0.0f;
	
	//setTransforms();
	//setAnimations();
}

void Duck::move(uint8_t up, uint8_t down, uint8_t left, uint8_t right, float elapsed)
{
		

    		//combine inputs into a move:
		playeranimtimer += elapsed;
		constexpr float PlayerSpeed = 20.0f;

		glm::vec2 move = glm::vec2(0.0f);
		if (left && !right) move.x =1.0f;
		if (!left && right) move.x = -1.0f;
		if (down& !up) move.y =1.0f;
		if (!down && up) move.y = -1.0f;

		if(move.x != 0 || move.y != 0) //run mode
		{
			//play the run animation and turn off idle animation
			{
				if(!(duckrotated))
				playeranimtimer = 1.0f;

				if(playeranimtimer >= (1.0f/duckrun.fps)) //go through run animation frames
				{
					playeranimtimer = 0;
					duckrun.currframe +=1;
					if(duckrun.currframe >= duckrun.frames.size())
					duckrun.currframe = 0;
				}

				for(int x =0; x<duckrun.frames.size(); x++)
				{
					for(int y =0; y<duckrun.frames[x].size(); y++)
					{
						if(duckrun.currframe == x ) duckrun.frames[x][y] -> scale = duckrun.scales[x][y];
						else duckrun.frames[x][y] -> scale = glm::vec3(0.0f);
					}
				}

				for(int x = 0; x < duckidle.numframes; x++) //turn off all duck idle frames
				{
					for(int y =0; y<duckidle.frames[x].size(); y++)
					{
						duckidle.frames[x][y]->scale = glm::vec3(0.0f);
					}
					
				}
			}
			//play the run animation and turn off idle animation

			//set the player and camera rotation
			{
				float rotateval = ((ducktransform.rotateholdx)/6.0f) * float(M_PI) * 2.0f;
				
				ducktransform.playertranslate->rotation =  glm::angleAxis(rotateval, glm::vec3(0.0f, 0.0f, 1.0f)) * ducktransform.pastrotation;
				ducktransform.pastrotation = ducktransform.playertranslate->rotation;
				ducktransform.rotateholdx = 0.0f;
				
				
				if(move.y >0.0f)
					*ducktransform.animrot = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				if(move.y < 0.0f)
					*ducktransform.animrot = glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				
				if(move.x > 0.0f)
				{
					*ducktransform.animrot  = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

					if(move.y > 0.0f)
					*ducktransform.animrot  = glm::angleAxis(glm::radians(90.0f + 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					if(move.y < 0.0f)
					*ducktransform.animrot  = glm::angleAxis(glm::radians(90.0f - 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

				}
				if(move.x < 0.0f)
				{
					*ducktransform.animrot  = glm::angleAxis(glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)); 

					if(move.y > 0.0f)
					*ducktransform.animrot  = glm::angleAxis(glm::radians(270.0f - 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					if(move.y < 0.0f)
					*ducktransform.animrot  = glm::angleAxis(glm::radians(270.0f + 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				}
			}
			//set the player and camera rotation
			duckrotated = true; //turn on duck rotated for idle mode
		}
		else //idle mode
		{
			//play the idle animation and turn off run animation
			{
				if(duckrotated)
				playeranimtimer = 1.0f;
			
				
				if(playeranimtimer >= (1.0f/duckidle.fps))
				{
					playeranimtimer = 0;
					duckidle.currframe +=1;
					if(duckidle.currframe >= duckidle.frames.size())
					duckidle.currframe = 0;

				}

				for(int x =0; x<duckidle.frames.size(); x++)
				{
					for(int y =0; y<duckidle.frames[x].size(); y++)
					{
						if(duckidle.currframe == x ) duckidle.frames[x][y] -> scale = duckidle.scales[x][y];
						else duckidle.frames[x][y] -> scale = glm::vec3(0.0f);
					}
					
				}

				for(int x = 0; x < duckrun.numframes; x++) //duck idle
				{
					for(int y =0; y<duckrun.frames[x].size(); y++)
					{
						duckrun.frames[x][y]->scale = glm::vec3(0.0f);
					}
					
				}
			}
			//play the idle animation and turn off run animation
			duckrotated = false;
		}
		

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		
		//trycamrotate sometime!
		glm::vec3 remain = duck.transform ->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:

		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			
			playerwalkmesh->walk_in_triangle(duck.at, remain, &end, &time);
			
			
			duck.at = end;
			
				
			if (time == 1.0f) {
				//finished within triangle:
				
			
				remain = glm::vec3(0.0f);
		
				break;
			}
			//some step remains:
			
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (playerwalkmesh->cross_edge(duck.at, &end, &rotation)) {
				//stepped to a new triangle:
				duck.at = end;
				
				//rotate step to follow surface:
				remain = rotation * remain;
			} else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const &a = playerwalkmesh->vertices[duck.at.indices.x];
				glm::vec3 const &b = playerwalkmesh->vertices[duck.at.indices.y];
				glm::vec3 const &c = playerwalkmesh->vertices[duck.at.indices.z];
				glm::vec3 along = glm::normalize(b-a);
				glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
				glm::vec3 in = glm::cross(normal, along);

				//check how much 'remain' is pointing out of the triangle:
				float d = glm::dot(remain, in);
				if (d < 0.0f) {
					//bounce off of the wall:
					remain += (-1.25f * d) * in;
				} else {
					//if it's just pointing along the edge, bend slightly away from wall:
					remain += 0.01f * d * in;
				}
			}
		}

		if (remain != glm::vec3(0.0f)) {
			//std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		
		ducktransform.playertranslate->position = playerwalkmesh->to_world_point(duck.at);


		{ //update player's rotation to respect local (smooth) up-vector:
			
			
			glm::quat adjust = glm::rotation(
				duck.transform ->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				playerwalkmesh->to_world_smooth_normal(duck.at) //smoothed up vector at walk location
			);

			duck.transform->rotation = glm::normalize(adjust * duck.transform->rotation);
			

		}  

}

void Duck::setTransforms()
{
	
    for (auto &transform : playerscene.transforms) { //get the player and the animation rotation transforms from the scene
		if (transform.name.find("Player") != -1) 
			ducktransform.playertranslate = &transform;
		else if(transform.name.find("animrot") != -1) 
		{
			
			Scene::Transform*temp = &transform;
			ducktransform.animrot =  &temp->rotation;
		}			
	} 

	

    //SET ALL TRANSFORMATIONS FOR USE LATER ON//

	playerscene.transforms.emplace_back();
	playerscene.cameras.emplace_back(&playerscene.transforms.back());
	duck.camera = &playerscene.cameras.back();
	duck.camera->fovy = glm::radians(60.0f);
	duck.camera->near = 0.01f;

	

	playerscene.transforms.emplace_back();
	duck.transform = &playerscene.transforms.back();


	duck.transform->parent = ducktransform.playertranslate;

	//start player walking at nearest walk point:
	duck.at = playerwalkmesh->nearest_walk_point(ducktransform.playertranslate->position);

	
	ducktransform.pastrotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	std::cout << "DEBUG -- MADE IT TO END" << std::endl;

    //SET ALL TRANSFORMATIONS FOR USE LATER ON//
	
}

void Duck::setAnimations()
{
    duckrun.numframes = 13;

	for(int x =0; x< duckrun.numframes; x++) //number of total frames for duckrun
	{
		std::vector<Scene::Transform*> ntrans;
		duckrun.frames.emplace_back(ntrans);
		std::vector<glm::vec3> nscl;
		duckrun.scales.emplace_back(nscl);
		
	} 

	for(int x = 0; x < duckrun.numframes; x++) //duck idle
	{
		for (auto &transform : playerscene.transforms) {
			std::string frm = "frame";
			int currfrm = x+ 1;
			frm += std::to_string(currfrm);
			
			if (transform.name.find(frm) != -1) 
			{
				if(transform.name.find("run") != -1)
				{
					Scene::Transform *temp = nullptr;
					temp = &transform;
					glm::vec3 tempscl = temp ->scale;
					duckrun.frames[x].emplace_back(temp);
					duckrun.scales[x].emplace_back(tempscl);
				}

			}
		}

	}

	

	for(int x = 0; x < duckrun.numframes; x++) //duck idle
	{
		for(int y =0; y<duckrun.frames[x].size(); y++)
		{
			duckrun.frames[x][y]->scale = glm::vec3(0.0f);
		}
		
	}



	duckidle.numframes = 14;

	for(int x =0; x< duckidle.numframes; x++) //number of total frames for duckrun
	{
		std::vector<Scene::Transform*> ntrans;
		duckidle.frames.emplace_back(ntrans);
		std::vector<glm::vec3> nscl;
		duckidle.scales.emplace_back(nscl);
		
	} 

	for(int x = 0; x < duckidle.numframes; x++) //duck idle
	{
		for (auto &transform : playerscene.transforms) {
			std::string frm = "frame";
			int currfrm = x+ 1;
			frm += std::to_string(currfrm);
			
			if (transform.name.find(frm) != -1) 
			{
				if(transform.name.find("idle")!= -1)
				{
					Scene::Transform *temp = nullptr;
					temp = &transform;
					glm::vec3 tempscl = temp ->scale;
					duckidle.frames[x].emplace_back(temp);
					duckidle.scales[x].emplace_back(tempscl);
				}

			}
		}

	}
}