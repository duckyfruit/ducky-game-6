#include "LitColorTextureProgram.hpp"

#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <random>
#include <array>


#include <hb.h>
#include <hb-ot.h>

#include <ft2build.h>
#include FT_FREETYPE_H

//Gameplan; render out the scene and yourself first; then emplace back drawables of all the players
//and their locations

//CREATE THE BACKGROUND SCENE AND YOURSELF//
void setfont(char* test, glm::vec3 anchor,glm::uvec2 const &drawable_size, glm::vec4 color, float boxwidth, float boxheight, bool scrolldialogue, int &scrolldialogueindex) //display text of the specified index
{
	glDisable(GL_DEPTH_TEST);
	float aspect = float(drawable_size.x) / float(drawable_size.y);
	DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
	));

	char *fontfile;
	char *text;
	
	const float pixelwidth = 128.0f;
	const float resolution = 1024.0f;
	const float beglinex = anchor.x;
	const float begliney = anchor.y;

	fontfile = "PressStart.ttf";
	text = test;


	FT_Library  library;   /* handle to library     */
	FT_Face     face;  
	 
	FT_Init_FreeType( &library );
	FT_New_Face( library, fontfile, 0, &face );
	
	FT_Set_Pixel_Sizes( face, uint8_t(pixelwidth), uint8_t(pixelwidth));   /* pixel_height          */
	FT_GlyphSlot  slot = face->glyph;

	FT_Select_Charmap(
          face,               /* target face object */
          FT_ENCODING_BIG5 );

	//Look into freetype
	
	/* And converted to absolute positions. */
	{
		
		for (int i = 0; i < strlen(test); i++)
		{
			if(!scrolldialogue)
			{
				FT_Load_Char(face,test[i], FT_LOAD_RENDER); //load the character
				if(test[i] == '\n') //newline!
				{
					if((i != strlen(test) - 1))
					{
						anchor.y -= (1/resolution *pixelwidth)*1.0f;
						anchor.x = beglinex;
					}
					
					FT_Load_Char( face, 32 , FT_LOAD_RENDER); //load a space instead
					
				}

				
				float origX = anchor.x;
				float origY = anchor.y;
				
				for(int y = (slot->bitmap.rows - 1); y >= 0 ; y--)
				{
					for(uint32_t x = 0; x < (slot->bitmap.width); x++)
					{
						if(slot->bitmap.buffer[y*slot->bitmap.width+ x])
						{
							glm::vec3 anchordup1 = anchor;
							glm::vec3 anchordup2 = anchor;
							glm::vec3 anchordup3 = anchor;
							anchordup1.x += 1/resolution;
							anchordup2.y += 1/resolution;
							anchordup3.x += 1/resolution;
							anchordup3.y += 1/resolution;

							glm::mat4x3 mat(anchor,anchordup1,anchordup2,anchordup3);
							
							lines.draw_rect(mat,color);
						}
						
						
						anchor.x += 1/resolution;
						
					}
					anchor.y += 1/resolution;
					anchor.x = origX;
				
				}

				anchor.x += (1/resolution *slot->bitmap.width)*1.0f + 1/resolution * 4.0f;
				anchor.y = origY;

				//if we wanted to shake, move y in a sinusoid function

				if(test[i] == ' ') //space!
				{
					anchor.x += (1/resolution *pixelwidth)/2.0f;	
					
				}

				if(abs(anchor.x) - abs(beglinex)> boxwidth) //if we have extended past the width of the box
				{
					anchor.y -= (1/resolution *pixelwidth)*1.0f;
					anchor.x = beglinex;	
				}

				
				if(abs(anchor.y)- abs(begliney)> boxheight) //if we have extended past the height of the box
				{
					//set pause render at the index of the string
					//when space is pressed resume at the index
					
					scrolldialogue = true;
					scrolldialogueindex = i;
					anchor.y = begliney;
					anchor.x = beglinex;
				}
	
			}
		}
		scrolldialogue = true;
	}

	FT_Done_Face    ( face );
  	FT_Done_FreeType( library );

}

GLuint background_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > background_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("duckyanimations.pnct"));
	background_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

GLuint ducky_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > ducky_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("duckyframes.pnct"));
	ducky_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

WalkMesh const *walkmesh = nullptr;
Load< WalkMeshes > phonebank_walkmeshes(LoadTagDefault, []() -> WalkMeshes const * {
	WalkMeshes *ret = new WalkMeshes(data_path("duckyanimations.w"));
	walkmesh = &ret->lookup("WalkMesh");
	return ret;
});

Load< Scene > ducky_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("duckyanimations.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = background_meshes->lookup(mesh_name);

		//make a new title for drawable; character!
		//if character drawable, emplace back transforms and do everything regularly,
		//but send the character information to the server
		//server will send back to players and 
		if(mesh_name.find("nonrender") == -1)
		{
			scene.drawables.emplace_back(transform);
			Scene::Drawable &drawable = scene.drawables.back();

			drawable.pipeline = lit_color_texture_program_pipeline;

			drawable.pipeline.vao = background_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type; 
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		}


	});
});

PlayMode::PlayMode(Client &client_) : client(client_), scene(*ducky_scene) {
	
	//For every mesh with the tag "apple"
	//put transform back and int (1) in a vector
	//if the int is 1, that means the apple has not been eaten
	//if the int is 0, that means the apple has been eaten
	//set the scale of that transform to 0 for all players
	//increment the score 
	//winner is whoever eats the most apples when all apples are gone
	
	
	
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
		for (auto &transform : scene.transforms) {
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

	

	for(int x = 0; x < duckrun.numframes; x++) //duck run
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
		for (auto &transform : scene.transforms) {
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

	for (auto &transform : scene.transforms) {
		if (transform.name.find("Player") != -1) 
			playertranslate = &transform;
		else if(transform.name.find("animrot") != -1) 
		{
			Scene::Transform*temp = &transform;
			animrot =  &temp->rotation;
		}
		else if(transform.name.find("Apple") != -1) 
		{
			applepos.emplace_back(&transform);
			apples.emplace_back(uint8_t(1));
		}
			
	} 


	//for the transforms in the scene
	//if transform.name.find("apple")
	//emplace back transform in applepos vector
	//emplace back int 1 in apple vetor

	scene.transforms.emplace_back();
	scene.cameras.emplace_back(&scene.transforms.back());
	player.camera = &scene.cameras.back();
	player.camera->fovy = glm::radians(60.0f);
	player.camera->near = 0.01f;

	scene.transforms.emplace_back();
	player.transform = &scene.transforms.back();

	scene.transforms.emplace_back();
	camrotate = &scene.transforms.back();

	scene.transforms.emplace_back();
	camtranslate = &scene.transforms.back();


	player.transform->parent = playertranslate;
	camrotate->parent = playertranslate;
	camtranslate->parent = camrotate;
	//player's eyes are 1.8 units above the ground:
	camtranslate->position = glm::vec3(0.0f, 25.0f, 60.0f);

	rotatecam = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	
	//rotate camera facing direction (-z) to player facing direction (+y):
	camrotate->rotation = rotatecam;
	player.camera->transform = camtranslate;
	
	//start player walking at nearest walk point:
	player.at = walkmesh->nearest_walk_point(playertranslate->position);

	pastrotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	rotateholdx = 0.0f;
	//*animrot = glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		}if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.downs += 1;
			controls.jump.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
	}
	}else if (evt.type == SDL_MOUSEMOTION) {
		
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			//controls.motion = evt.motion.xrel / float(window_size.y); //get the motion of the mouse
			//rotateholdx = controls.motion;
			glm::vec3 upDir(0.0f,0.0f, 1.0f);
			if(rotateholdx >= 6.0f || rotateholdx <= -6.0f)
			rotateholdx = 0.0f;

			rotateholdx -= evt.motion.xrel/ float(window_size.y); 
		
			/* float pitch = glm::pitch(player.camera->transform->rotation);
			pitch += motion.y * player.camera->fovy;
			//camera looks down -z (basically at the player's feet) when pitch is at zero.
			pitch = std::min(pitch, 0.95f * 3.1415926f);
			pitch = std::max(pitch, 0.05f * 3.1415926f); */

			
			duckheadrotate =  glm::angleAxis(rotateholdx * player.camera->fovy, upDir)  * rotatecam;
			camrotate->rotation = duckheadrotate;
			
			//player.camera->transform->rotation = rotatecam;

			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	controls.ateapple = 0;
	controls.applenum = -1;
	//player walking:
	{
		//combine inputs into a move:
		playeranimtimer += elapsed;

		animtype = 0;
		constexpr float PlayerSpeed = 20.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (controls.left.pressed && !controls.right.pressed) move.x =1.0f;
		if (!controls.left.pressed && controls.right.pressed) move.x = -1.0f;
		if (controls.down.pressed && !controls.up.pressed) move.y =1.0f;
		if (!controls.down.pressed && controls.up.pressed) move.y = -1.0f;

		if(move.x != 0 || move.y != 0)
		{
			animtype = 1;
			if(!duckrotated)
			playeranimtimer = 1.0f;

			if(playeranimtimer >= (1.0f/duckrun.fps)) //make duck run!
			{
				playeranimtimer = 0;
				duckrun.currframe +=1;
				if(duckrun.currframe >= duckrun.frames.size())
				duckrun.currframe = 0;
				animframe = duckrun.currframe;
			}

			for(int x =0; x<duckrun.frames.size(); x++)
			{
				for(int y =0; y<duckrun.frames[x].size(); y++)
				{
					if(duckrun.currframe == x ) duckrun.frames[x][y] -> scale = duckrun.scales[x][y];
					else duckrun.frames[x][y] -> scale = glm::vec3(0.0f);
				}
			}

			for(int x = 0; x < duckidle.numframes; x++) //duck idle
			{
				for(int y =0; y<duckidle.frames[x].size(); y++)
				{
					duckidle.frames[x][y]->scale = glm::vec3(0.0f);
				}
				
			} 

	
			float rotateval = ((rotateholdx)/6.0f) * float(M_PI) * 2.0f;
			
	
			playertranslate->rotation =  glm::angleAxis(rotateval, glm::vec3(0.0f, 0.0f, 1.0f)) * pastrotation;
			pastrotation = playertranslate->rotation;
			rotateholdx = 0.0f;
			camrotate->rotation = rotatecam;
		
			
			duckrotated = true;

		
			if(move.y >0.0f)
			{
				*animrot = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			if(move.y < 0.0f)
			{
				*animrot  = glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			
			if(move.x > 0.0f)
			{
				*animrot  = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

				if(move.y > 0.0f)
				*animrot  = glm::angleAxis(glm::radians(90.0f + 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				if(move.y < 0.0f)
				*animrot  = glm::angleAxis(glm::radians(90.0f - 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

			}
			if(move.x < 0.0f)
			{
				*animrot  = glm::angleAxis(glm::radians(270.0f), glm::vec3(0.0f, 0.0f, 1.0f)); 

				if(move.y > 0.0f)
				*animrot  = glm::angleAxis(glm::radians(270.0f - 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				if(move.y < 0.0f)
				*animrot  = glm::angleAxis(glm::radians(270.0f + 45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			}
			
			
		}
		else
		{
			animtype = 0;
			if(duckrotated)
				playeranimtimer = 1.0f;
		
			
			if(playeranimtimer >= (1.0f/duckidle.fps))
			{
				playeranimtimer = 0;
				duckidle.currframe +=1;
				if(duckidle.currframe >= duckidle.frames.size())
				duckidle.currframe = 0;
				animframe = duckidle.currframe;

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

			duckrotated = false;
		} 
		

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		//get move in world coordinate system:
		
		//trycamrotate sometime!
		glm::vec3 remain = player.transform ->make_local_to_world() * glm::vec4(move.x, move.y, 0.0f, 0.0f);

		//using a for() instead of a while() here so that if walkpoint gets stuck in
		// some awkward case, code will not infinite loop:


		for (uint32_t iter = 0; iter < 10; ++iter) {
			if (remain == glm::vec3(0.0f)) break;
			WalkPoint end;
			float time;
			
			walkmesh->walk_in_triangle(player.at, remain, &end, &time);
			
			
			player.at = end;
			
				
			if (time == 1.0f) {
				//finished within triangle:
				
			
				remain = glm::vec3(0.0f);
		
				break;
			}
			//some step remains:
			
			remain *= (1.0f - time);
			//try to step over edge:
			glm::quat rotation;
			if (walkmesh->cross_edge(player.at, &end, &rotation)) {
				//stepped to a new triangle:
				player.at = end;
				
				//rotate step to follow surface:
				remain = rotation * remain;
			} else {
				//ran into a wall, bounce / slide along it:
				glm::vec3 const &a = walkmesh->vertices[player.at.indices.x];
				glm::vec3 const &b = walkmesh->vertices[player.at.indices.y];
				glm::vec3 const &c = walkmesh->vertices[player.at.indices.z];
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
			std::cout << "NOTE: code used full iteration budget for walking." << std::endl;
		}

		//update player's position to respect walking:
		
		playertranslate->position = walkmesh->to_world_point(player.at);


		{ //update player's rotation to respect local (smooth) up-vector:
			
			
			glm::quat adjust = glm::rotation(
				player.transform ->rotation * glm::vec3(0.0f, 0.0f, 1.0f), //current up vector
				walkmesh->to_world_smooth_normal(player.at) //smoothed up vector at walk location
			);

			player.transform->rotation = glm::normalize(adjust * player.transform->rotation);
			

		}  

	}
	
	//player touches apple
	{
		for(int i = 0; i < applepos.size(); i++)
		{
			float radius = 20.0f;
			if(playertranslate->position.x > applepos[i]->position.x - radius &&
			playertranslate->position.x < applepos[i]->position.x + radius &&
			playertranslate->position.y > applepos[i]->position.y - radius &&
			playertranslate->position.y < applepos[i]->position.y + radius&& 
			apples[i] == 1 )
			{
				apples[i] = uint8_t(0);
				applepos[i]->scale = glm::vec3(0.0f);
				std::cout << glm::to_string(playertranslate->position) << std::endl;
				std::cout << glm::to_string(applepos[i]->position) << std::endl;
				std::cout<<std::endl;
				controls.ateapple = 1;
				controls.applenum = i;
				score += 1;
			}
		}
	} 
	
	//if player position is within range of apple & apple is not 0 in apple vector
	//make apple 0 in apple vector 
	//set scale apple to 0
	
	
	controls.playerpos = playertranslate->position;
	controls.playerrot = *animrot * playertranslate -> rotation;
	controls.playeranim = animtype;
	controls.playerframe = animframe;
	

	



	//controls.apple = apple vector
	//send apple vector to server
	//queue data for sending to server:
	

	controls.send_controls_message(&client.connection);

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.jump.downs = 0;
	controls.ateapple = 0;
	controls.applenum = -1;



	
	//send/receive data:
	
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0); 

	//std::cout << controls.motion << std::endl;

	//std::cout << "made it to apple playmode" << std::endl; 
	
	//for &p : game.players
	//for all apples in apple vector
	//set apples with int 0 at a scale of 0
	//set client apple vector to player apple vector (& operation)
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	//for draw, we want to emplace_back drawables that might be added to the scene (such as players)
	//so each player should keep track of its mesh
	int pcount = 0;

	for (auto const &p : game.players) {
		
		if(pcount > 0)
		{
			
			Mesh mesh;
			std::string getframe;

			int frame = p.playerframe + 1;
			std::cout << frame << std::endl;
			if(p.playeranim == 1)
			{
				if(frame > 13)
				frame = 13;
				getframe = "runanimframe" + std::to_string(frame);
			}
			else
			{
				if(frame > 14)
				frame = 14;
				getframe = "idleanimframe" + std::to_string(frame);
				
			}
				
			mesh = ducky_meshes->lookup(getframe);
			
			scene.transforms.emplace_back();
			scene.transforms.back().position = p.pos;
			scene.transforms.back().rotation = p.rot;
			{
				scene.drawables.emplace_back(&scene.transforms.back());
				Scene::Drawable &drawable = scene.drawables.back();
				drawable.pipeline = lit_color_texture_program_pipeline;
				drawable.pipeline.vao = ducky_meshes_for_lit_color_texture_program; 
				drawable.pipeline.type = mesh.type; 
				drawable.pipeline.start = mesh.start;
				drawable.pipeline.count = mesh.count;
			}

		
			if(p.ateapple == 1)
			{
				if(p.applenum >= 0)
				{
					apples[p.applenum] = 0;
					applepos[p.applenum]->scale = glm::vec3(0.0f);

				}
				
			} 
			
		


		}
		pcount ++;
		
	}

	player.camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f,1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*player.camera);


	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		glm::vec4 textcolor(0x00,0x00,0x00,0xff); //black text color
		int noscroll = 0;
		setfont(&std::to_string(score)[0],glm::vec3(0.0f - H, 0.75f, 0.0), drawable_size,textcolor, 1.0f, 1.0f,0,noscroll);
	}

	for(int i = 0; i < pcount - 1; i++)
	{
		scene.drawables.pop_back();
		scene.transforms.pop_back();

	}
	


	//can you draw multiple scenes on top of one another? if so, draw each player scene to 
	//get each player's character into the client scene with respoect to the client camera

	GL_ERRORS();
}
