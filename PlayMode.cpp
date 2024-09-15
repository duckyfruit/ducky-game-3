#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint alien_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > alien_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("alien.pnct"));
	alien_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > alien_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("alien.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = alien_meshes->lookup(mesh_name);

		if(mesh_name == "Helmet")
		{
			scene.drawables.emplace_front(transform);
			Scene::Drawable &drawable = scene.drawables.front();
			drawable.pipeline = lit_color_texture_program_pipeline;
			
			drawable.pipeline.vao = alien_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;

			drawable.pipeline.trans = 1; //set transparency to 1
		}
		else
		{
			scene.drawables.emplace_back(transform);
			Scene::Drawable &drawable = scene.drawables.back();
			drawable.pipeline = lit_color_texture_program_pipeline;
			drawable.pipeline.vao = alien_meshes_for_lit_color_texture_program;
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
		}

		

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});

Load< Sound::Sample > tatercry(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("Pickup_coin 25v2.wav"));
});

PlayMode::PlayMode() : scene(*alien_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Head") head = &transform;
		else if (transform.name == "Body") body = &transform;
		else if (transform.name == "Stem") plant = &transform;
		else if (transform.name == "Mouth") closedmouth = &transform;
		else if (transform.name == "Mouth2") openmouth = &transform;
	}
	if (head == nullptr) throw std::runtime_error("Head not found.");
	if (body == nullptr) throw std::runtime_error("Body not found.");
	if (plant == nullptr) throw std::runtime_error("Stem not found.");
	head_rotation = head->rotation;
	body_rotation = body->rotation;
	plant_rotation = plant->rotation;
	openmouth -> scale = glm::vec3(0.0f, 0.0f, 0.0f);

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	head_tip_loop = Sound::play_3D(*tatercry, 1.0f, get_head_tip_position(), 10.0f);
	//start music loop playing:
	// (note: position will be over-ridden in update())
	
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			hup.downs += 1;
			hup.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN) {
			hdown.downs += 1;
			hdown.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.downs += 1;
			space.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			hup.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN) {
			hdown.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void squeak(float &squash, float elapsed, Scene::Transform *closedmouth, Scene::Transform *openmouth, Scene::Transform *body)
{
	if(squash >= 0.0f) //squash animation
	{
		squash -= elapsed;
		openmouth -> scale = glm::vec3(1.0f, 1.0f, 1.0f);
		closedmouth -> scale = glm::vec3(0.0f, 0.0f, 0.0f);
		body -> scale.y = 0.25f * abs(sin( float(M_PI) *squash * 1.5f)) + 0.75f;
		if(squash < 2.25f)
		{
			squash = -1.0f;
			openmouth -> scale = glm::vec3(0.0f, 0.0f, 0.0f);
			closedmouth -> scale = glm::vec3(1.0f, 1.0f, 1.0f);
		}
		
	}

}

void wander(Scene::Transform *body, glm::vec3 &body_rotation, float randrotate, float &randtimer, float &randzmov, float elapsed)
{
	if(randtimer <= 0) //if it is no longer rotating
	{
		body -> position.z += 0.25f *randzmov;
		randzmov -= elapsed;
	}	
	else//if it is rotating
	{
		body -> rotation = body_rotation * glm::angleAxis(
		glm::radians(randrotate * float(M_PI)),
		glm::vec3(0.0f, 1.0f, 0.0f ));
		randtimer -= elapsed;
	}
}


void PlayMode::update(float elapsed) {

	wobble += elapsed / 10.0f;
	movtimer += elapsed;
	if(record) //if it is recording increment the record timer 
	rectime += elapsed;
	
	if(!record) //if it is not recording increment the timer
	timer += elapsed;

	if(int(movtimer) % 10 == 0) //if it has been 10 seconds
	{
		randrotate = ((rand()) / static_cast <float> (RAND_MAX)); //set randrotate to a random value
		rotatetimer =  ((rand()) / static_cast <float> (RAND_MAX)) * 2.0f;
	}
	

	body -> rotation = body_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 5.0f  * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f ));
	plant -> rotation = plant_rotation * glm::angleAxis(
		glm::radians(15.0f * std::sin(wobble * 5.0f  * float(M_PI))),
		glm::vec3(1.0f, 0.0f, 0.0f ));

	if(!record && !soundrec.empty()) //if the record button is not pressed and soundrec is not empty
	{
		
		auto it = soundrec.begin(); //iterator that points to the first element
		if(timer >= *it) //if timer is greater than or equal to this element
		{
			head_tip_loop = Sound::play_3D(*tatercry, 1.0f, get_head_tip_position(), 10.0f); //play the sound
			float sendback = *it;
			soundrec.erase(it);
			soundrec.emplace_back(sendback); //erase the time from the front and send it to the back
			timer = 0.0f;
			squash = 2.5f;
		}
		

	}



	if(space.pressed && squash == -1.0f) //if you squish the creature
	{
		squash = 2.5f;

		head_tip_loop = Sound::play_3D(*tatercry, 1.0f, get_head_tip_position(), 10.0f);

		if(record)
		{
			soundrec.emplace_back(rectime); //put it back in soundrec
			rectime = 0.0f; //reset the recording time
		}
		
	}

	if(hdown.pressed) //if the record button is pressed, set the flags
	{
		if(!record) //if it is not already recording, begin to record
		{
			record = 1;
			rectime = 0.0f;
			soundrec.clear(); //clear the sound list
		}
	}
	if(hup.pressed)
	{
		if(record) //if it is already recording end the record
		{
			record = 0;
			timer = 0.0f;
		}
	}
	
	squeak(squash,elapsed,closedmouth,openmouth,body);

	
	//move sound to follow leg tip position:
	head_tip_loop->set_position(get_head_tip_position(), 1.0f / 60.0f); 

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}


	//if 1 is pressed create a new monster 


	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	space.downs = 0;
	hdown.downs = 0;
	hup.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(-1.0f, -1.0f,1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);


	glUseProgram(lit_color_texture_program->flatprogram);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(-1.0f, -1.0f,1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	
	glClearColor(0.094f, 0.043f, 0.38f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

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
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_head_tip_position() {
	//the vertex position here was read from the model in blender:
	return head->make_local_to_world() * glm::vec4(1.98215f, 1.73985f, 0.0f, 1.0f);
}
