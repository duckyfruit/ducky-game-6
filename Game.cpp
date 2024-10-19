#include "Game.hpp"

#include "Connection.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"



#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <random>

#include <glm/gtx/norm.hpp>



void Player::Controls::send_controls_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 36;

	
	connection.send(Message::C2S_Controls);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		if (b.downs & 0x80) {
			std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
		}
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	connection.send(playerpos.x);
	connection.send(playerpos.y);
	connection.send(playerpos.z);
	connection.send(playerrot.w);
	connection.send(playerrot.x);
	connection.send(playerrot.y);
	connection.send(playerrot.z); 
	connection.send(playeranim);
	connection.send(playerframe);

}

bool Player::Controls::recv_controls_message(Connection *connection_) {
	
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;


	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Controls)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 36) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 48!");
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	auto recv_button = [](uint8_t byte, Button *button) {
		button->pressed = (byte & 0x80);
		uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
		if (d > 255) {
			std::cerr << "got a whole lot of downs" << std::endl;
			d = 255;
		}
		button->downs = uint8_t(d);
	};


	//std::cout << "hello!!" <<std::endl;


	float* xpos = reinterpret_cast<float*>(&recv_buffer[4+0]);
	playerpos.x = *xpos;
	float* ypos = reinterpret_cast<float*>(&recv_buffer[4+4]);
	playerpos.y = *ypos;
	float* zpos = reinterpret_cast<float*>(&recv_buffer[4+8]);
	playerpos.z = *zpos;

	float* wrot = reinterpret_cast<float*>(&recv_buffer[4+12]);
	playerrot.w = *wrot;
	float* xrot = reinterpret_cast<float*>(&recv_buffer[4+16]);
	playerrot.x = *xrot;
	float* yrot = reinterpret_cast<float*>(&recv_buffer[4+20]);
	playerrot.y = *yrot;
	float* zrot = reinterpret_cast<float*>(&recv_buffer[4+24]);
	playerrot.z = *zrot;

	int* anim = reinterpret_cast<int*>(&recv_buffer[4+28]);
	playeranim = *anim;
	int* frame = reinterpret_cast<int*>(&recv_buffer[4+32]);
	playerframe = *frame;




	

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}


//-----------------------------------------


Game::Game() : mt(0x15466666) {

}

Player *Game::spawn_player() {
	
	players.emplace_back();
	Player &player = players.back();
	return &player;
}

void Game::remove_player(Player *player) {
	
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			players.erase(pi);
			found = true;
			break;
		}
	}
	assert(found);
}

void Game::update(float elapsed) {

	
	


}


void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	
	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer

	//helper function for sending the scene over 


	auto send_player = [&](Player const &player) {
		
		
		glm::highp_quat matrixrot = player.controls.playerrot;
		glm::vec3 xyzpos = player.controls.playerpos;

		connection.send(xyzpos); //12 BYTES
		connection.send(matrixrot);//16 BYTES
		connection.send(player.controls.playeranim);
		connection.send(player.controls.playerframe);

	
	};

	//player count:
	connection.send(uint8_t(players.size()));
	if (connection_player) send_player(*connection_player);
	for (auto const &player : players) {
		if (&player == connection_player) continue;
		send_player(player);
	}

	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;
	


	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_State)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	uint32_t at = 0;
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;




	//copy bytes from buffer and advance position:
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			std::cout<< int(at + sizeof(*val)) << std::endl;
			std::cout << size<< std::endl;
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	players.clear();
	uint8_t player_count;
	read(&player_count);
	for (uint8_t i = 0; i < player_count; ++i) {
		players.emplace_back();
		Player &player = players.back();
		
		read(&player.pos);
		read(&player.rot);
		read(&player.playeranim);
		read(&player.playerframe);

	
		
	}

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
