#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, space, hdown, hup;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	std::vector<float> soundrec;
	//hexapod leg to wobble:
	Scene::Transform *body = nullptr;
	Scene::Transform *head = nullptr;
	Scene::Transform *openmouth = nullptr;
	Scene::Transform *closedmouth = nullptr;
	Scene::Transform *plant = nullptr;
	glm::quat head_rotation;
	glm::quat body_rotation;
	glm::quat plant_rotation;

	float wobble = 0.0f;
	float squash = -1.0f;

	float randrotate = 0.0f;
	float randzmov = 0.0f;

	glm::vec3 get_head_tip_position();

	int record = 0;
	float rectime = 0.0f;
	float timer = 0.0f;
	float movtimer = 0.0f;
	float rotatetimer = 0.0f;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > head_tip_loop;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
