#define GLFW_EXPOSE_NATIVE_WIN32
#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION  
#define ZEN_H_IMPLEMENTATION
#define ZEN_MATH_IMPLEMENTATION
#define ZEN_GL_IMPLEMENTATION
#define ZEN_GLFW_IMPLEMENTATION
#define ZEN_IMGUI_IMPLEMENTATION
#define ZEN_BOX2D_IMPLEMENTATION
#define GB_STRING_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "stb/stb_image.h"
#include "stb/stb_truetype.h"
#include "gb/gb_string.h"

#include "imgui/imgui.h"
#include "zen/zen.h"
#include "zen/zen_math.h"
#include "zen/zen_gl.h"
#include "zen/zen_glfw.h"
#include "zen/zen_imgui.h"
#include "Box2D/Box2D.h"
#include "zen/zen_box2d.h"


#define check(v) do { \
	if (!(v)) { \
		zout(#v " %s:%d", __FILE__, __LINE__); \
		exit(EXIT_FAILURE); \
	} \
} while(0)

#define KEY_DOWN(v) glfw.keys[GLFW_KEY_##v]
#define KEY_DOWN_ONCE(v) glfw.keys[GLFW_KEY_##v] == 1

#define SHOW_TWEEKS 0

enum {
	NUM_PICKUPS = 10 
};


typedef struct Pickup_t {

	Transform2d_t transform;
	int active;
	ZGLTexture *texture;
	b2Body *collider;

} Pickup_t;


ZGLFW glfw = {0};
ZGLBasicState bs = {0};
ZGLTexture background_texture = {0};
Transform2d_t background_transform = {0};
ZGLTexture player_texture = {0};
Transform2d_t player_transform = {0};
ZGLTexture pickup_texture = {0};
Pickup_t pickups[NUM_PICKUPS] = {0};

Matrix4x4_t camera = Matrix4x4();

int score = 0;
int game_won = 0;
float last_rot = 0.0f;
float last_x = 0.0f, last_y = 0.0f;

float bs_width = 10.0, bs_height = 10.0;

float position[2] = {0.0f, 0.0f};
float linear_damping = 0.4;
float angular_damping = 0.1f;
float radius = .33f;
float density = 1.0f;
float friction = 0.1f;
float restitution = 0.4f;
float force = 3.0f;
float max_vel = 3.0f;

b2World *world;
b2Body *body = 0;
b2Body *walls[4] = {0};

void character_callback(GLFWwindow*, unsigned int c) {
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

void window_size_callback(GLFWwindow*, int width, int height) {

	glViewport(0, 0, width, height);

	float wf = width;
	float hf = height;
	bs_width = 10.0f;
	bs_height = bs_width * hf / wf;
	if (bs_height < 10.f) {
		bs_height = 10.0f;
		bs_width = bs_height * wf / hf;
	}

	g_camera.m_width = bs_width / 2.0f;
	g_camera.m_height = bs_height / 2.0f;

}

void recreate_body() {

	if (body) {
		world->DestroyBody(body);
		body = 0;
	}

	// create falling box
	b2BodyDef body_def;
	body_def.type = b2_dynamicBody;
	body_def.allowSleep = true;
	body_def.awake = true;
	body_def.position.Set(position[0], position[1]);
	body_def.angularDamping = angular_damping;
	body_def.linearDamping = linear_damping;
	body = world->CreateBody(&body_def);

	b2CircleShape shape;
	shape.m_radius = radius;

	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = density;
	fixture_def.friction = friction;
	fixture_def.restitution = restitution;

	body->CreateFixture(&fixture_def);

}

void reset() {
	body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
	body->SetAngularVelocity(0);
	body->SetTransform(b2Vec2(position[0], position[1]), 0);
}

void ufo_init() {
	
	check(zgl_load_texture2d_from_file(&background_texture, FALSE, "./sprites/Background.png"));
	check(zgl_load_texture2d_from_file(&player_texture, FALSE, "./sprites/UFO.png"));
	check(zgl_load_texture2d_from_file(&pickup_texture, FALSE, "./sprites/Pickup.png"));

	background_transform = Transform2d(Vector2(0, 0), 0, Vector2(9, 9));
	player_transform = Transform2d(Vector2(0, 0), 0, Vector2(1.0f, 1.0f));

	g_debugDraw.Create();

	// setup world
	b2Vec2 gravity(0.0f, 0.0f);
	world = new b2World(gravity);
	world->SetDebugDraw(&g_debugDraw);
	world->SetSubStepping(true);
	world->SetWarmStarting(true);
	world->SetAutoClearForces(true);

	// create walls
	b2BodyDef wall_def;
	wall_def.position.Set(4.0f, 0.0f);
	walls[0] = world->CreateBody(&wall_def);
	b2PolygonShape box;
	box.SetAsBox(0.5f, 5.0f);
	walls[0]->CreateFixture(&box, 0.0f);

	wall_def.position.Set(-4.0f, 0.0f);
	walls[1] = world->CreateBody(&wall_def);
	box.SetAsBox(0.5f, 5.0f);
	walls[1]->CreateFixture(&box, 0.0f);

	wall_def.position.Set(0.0f, 4.0f);
	walls[2] = world->CreateBody(&wall_def);
	box.SetAsBox(5.0f, 0.5f);
	walls[2]->CreateFixture(&box, 0.0f);

	wall_def.position.Set(0.0f, -4.0f);
	walls[3] = world->CreateBody(&wall_def);
	box.SetAsBox(5.0f, 0.5f);
	walls[3]->CreateFixture(&box, 0.0f);

	recreate_body();


	b2BodyDef body_def;
	body_def.type = b2_kinematicBody;

	b2PolygonShape shape;
	shape.SetAsBox(0.65f * 0.4, 0.45f * 0.4);

	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.isSensor = true;

	float radians = M_PI * 2.f / cast(float)(NUM_PICKUPS);
	float radius = 2.75f;
	for (int i = 0; i < NUM_PICKUPS; ++i) {
		float rot = i * radians;
		float x = radius * cosf(rot);
		float y = radius * sinf(rot);
		Vector2_t position = Vector2(x, y);
		float rotation = 0.0f; 
		Vector2_t scale = Vector2(0.65f, 0.45f);
		pickups[i].transform = Transform2d(position, rotation, scale);
		pickups[i].texture = &pickup_texture;
		pickups[i].active = 1;

		body_def.position.Set(x, y);
		pickups[i].collider = world->CreateBody(&body_def);

		pickups[i].collider->CreateFixture(&fixture_def);
		pickups[i].collider->SetUserData(&pickups[i]);

	}

}

void draw_sprite(ZGLTexture *texture, Transform2d_t transform) {

	Matrix4x4_t mat = Matrix4x4();
	mat = scale_mat4x4(mat, Vector3(transform.scale.x, transform.scale.y, 1.0f));
	mat = rotz_mat4x4(mat, transform.rotation);
	mat = trans_mat4x4(mat, Vector3(transform.position.x, transform.position.y, 0.0f));
	mat = mul_mat4x4(mat, camera);
	zgl_bs_draw_textured_rect(&bs, texture, mat.m, -0.5f, -0.5f, 1.0f, 1.0f, FALSE);

}

void fixed_update(double dt) {

	last_x = body->GetPosition().x;
	last_y = body->GetPosition().y;
	last_rot = body->GetAngle();
	int32 velocity_iterations = 8;
	int32 position_iterations = 3;
	world->Step(dt, velocity_iterations, position_iterations);

	b2ContactEdge *cl = body->GetContactList();
	while (cl) {
		void *data = cl->other->GetUserData();
		if (cl->contact->IsTouching() && data) {
			Pickup_t *pickup = (Pickup_t *)data;
			cl->other->SetActive(false);
			if (pickup->active) {
				pickup->active = false;
				score++;
				zout("Score: %d", score);
			}
		}
		cl = cl->next;
	}

}

void update(double dt) {
	ImGuiIO io = ImGui::GetIO();
	if (io.WantTextInput) return;

	if (KEY_DOWN_ONCE(R)) {
		reset();
	}

	bool wake = true;
	if (KEY_DOWN(W) || KEY_DOWN(UP)) {
		body->ApplyForceToCenter(b2Vec2(0, force), wake);
	}
	if (KEY_DOWN(A) || KEY_DOWN(LEFT)) {
		body->ApplyForceToCenter(b2Vec2(-force, 0), wake);
	}
	if (KEY_DOWN(S) || KEY_DOWN(DOWN)) {
		body->ApplyForceToCenter(b2Vec2(0, -force), wake);
	}
	if (KEY_DOWN(D) || KEY_DOWN(RIGHT)) {
		body->ApplyForceToCenter(b2Vec2(force, 0), wake);
	}

	// max force
	b2Vec2 vel = body->GetLinearVelocity();
	if (vel.Length() > max_vel) {
		vel.Normalize();
		vel *= max_vel;
		body->SetLinearVelocity(vel);
	}	

	Vector3_t pos = Vector3(-player_transform.position.x, -player_transform.position.y, 0.0f);
	camera = trans_mat4x4(Matrix4x4(), pos);
	camera = scale_mat4x4(camera, Vector3(1.5f, 1.5f, 1.0f));

	int active_pickups = 0;
	for (int i = 0; i < NUM_PICKUPS; ++i) {
		if (pickups[i].active) {
			pickups[i].transform.rotation += M_PI * 0.8f * glfw.delta_time;
			pickups[i].collider->SetTransform(pickups[i].collider->GetPosition(), pickups[i].transform.rotation);
			active_pickups++;
		}
	}

	if (active_pickups == 0) {
		game_won = true;
	}

}

void render(float alpha) {

	float x = body->GetPosition().x;
	float y = body->GetPosition().y;
	float rot = body->GetAngle();
	float dx = (1.0f - alpha) * last_x + alpha * x; 
	float dy = (1.0f - alpha) * last_y + alpha * y;
	float drot = (1.0f - alpha) * last_rot + alpha * rot;

	player_transform.position.x = dx;
	player_transform.position.y = dy;
	player_transform.rotation = drot;

	// draw the background
	draw_sprite(&background_texture, background_transform);
	draw_sprite(&player_texture, player_transform);

	for (int i = 0; i < NUM_PICKUPS; ++i) {
		if (pickups[i].active)
			draw_sprite(pickups[i].texture, pickups[i].transform);
	}

	uint32 flags = 0;
	//flags += b2Draw::e_shapeBit;
	//flags += b2Draw::e_jointBit;
	//flags += b2Draw::e_aabbBit;
	//flags += b2Draw::e_centerOfMassBit;
	g_debugDraw.SetFlags(flags);
	world->DrawDebugData();
	g_debugDraw.Flush();
}

int main(int, char **) {

	glfw = {0};
	glfw.window_title = "2D Ufo Example";
	glfw.major_version = 4;
	glfw.minor_version = 4;
	glfw.window_width = 1024;
	glfw.window_height = 768;
	glfw.window_cursor_mode = GLFW_CURSOR_NORMAL;
	glfw.window_background = 0xFF333333;
	glfw.window_size_callback = window_size_callback;
	glfw.character_callback = character_callback;

	zglfw_init(&glfw);
	zgl_bs_initialize(&bs);
	zen_imgui_init(&glfw);
	//ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	ufo_init();

	zglfw_show_window(&glfw);
	double accumulator = 0.0; 
	double dt = 1.0 / 60.0;
 
	while(zglfw_is_running(&glfw)) {

		zglfw_begin(&glfw);
		zgl_bs_begin(&bs, bs_width, bs_height);
		zen_imgui_begin(&glfw);

		glfw.should_close = glfw.keys[GLFW_KEY_ESCAPE];

		double frame_time = glfw.delta_time;
		if (frame_time > 0.25)
			frame_time = 0.25;

		accumulator += frame_time;
		while (accumulator >= dt) {
			fixed_update(dt);
			accumulator -= dt;
		}

		update(frame_time);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		render(accumulator / dt);

		ImGui::SetNextWindowPos(ImVec2(10,10));
		static bool open = true;
		int flags = 0;
		flags |= ImGuiWindowFlags_NoTitleBar;
		flags |= ImGuiWindowFlags_NoResize;
		flags |= ImGuiWindowFlags_NoMove;
		flags |= ImGuiWindowFlags_NoSavedSettings;
		flags |= ImGuiWindowFlags_AlwaysAutoResize;
		ImGui::PushFont(__zen_imgui_custion_font);
		ImGui::Begin("Example: Fixed Overlay", &open, ImVec2(0,0), .5f, flags);
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Score: %d", score);
		ImGui::End();


		if (game_won) {
			ImVec2 text_size = ImGui::CalcTextSize("You Win!!!", NULL, true);
			float textx = (glfw.window_width - text_size.x) * 0.5f;
			float texty = (glfw.window_height - text_size.y) * 0.4f;
			ImGui::SetNextWindowPos(ImVec2(textx, texty));
			ImGui::Begin("", &open, ImVec2(0, 0), 0.5f, flags);
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "You Win!!!");
			ImGui::End();
		}

		ImGui::PopFont();

		if (SHOW_TWEEKS) {
			flags = 0;
			flags |= ImGuiWindowFlags_AlwaysAutoResize;
			flags |= ImGuiWindowFlags_NoSavedSettings;
			ImGui::Begin("Tweeks and Berries", &open, ImVec2(0,0), 0.3f, flags);

			if (ImGui::DragFloat2("Position", position)) {
				if (!body->IsAwake()) {
					body->SetTransform(b2Vec2(position[0], position[1]), 0.0f);
				}
			}

			if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, FLT_MAX)) {
				recreate_body();
			}
			if (ImGui::DragFloat("Linear Damp", &linear_damping, 0.1f, 0.0f, FLT_MAX)) {
				recreate_body();
			}
			if (ImGui::DragFloat("Angular Damp", &angular_damping, 0.1f, 0.0f, FLT_MAX)) {
				recreate_body();
			}
			if (ImGui::SliderFloat("Density", &density, 0.0f, 10.0f)) {
				recreate_body();
			}
			if (ImGui::SliderFloat("Friction", &friction, 0.0f, 100.0f)) {
				recreate_body();
			}
			if (ImGui::SliderFloat("Bounce", &restitution, 0.0f, 1.0f)) {
				recreate_body();
			}
			if (ImGui::DragFloat("Force", &force, 1.f, 1.f, 100.f)) {
				recreate_body();
			}
			if (ImGui::DragFloat("Max Velocity", &max_vel, 0.1f, 0.1f, 100.0f)) {
				recreate_body();
			}

			ImGui::End();
		}

		zgl_bs_end(&bs);
		zen_imgui_end();
		zglfw_end(&glfw);
	}

	zout("\n%d (ms)", cast(int)(1000.0f / ImGui::GetIO().Framerate));

	g_debugDraw.Destroy();
	zgl_bs_end(&bs);
	zen_imgui_quit();
	zglfw_quit(&glfw);

	delete world;

	return 0;

}
