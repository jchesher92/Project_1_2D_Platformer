#include "engine/array_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "engine/global.h"
#include "engine/config.h"
#include "engine/input.h"
#include "engine/time.h"
#include "engine/physics.h"
#include "engine/util.h"
#include "engine/entity.h"
#include "engine/render.h"
#include "engine/animation.h"
#include "engine/audio.h"

void reset(void);

static Mix_Music *MUSIC_STAGE_1;
static Mix_Chunk *SOUND_JUMP;
static Mix_Chunk *SOUND_SHOOT;
static Mix_Chunk *SOUND_BULLET_HIT_WALL;
static Mix_Chunk *SOUND_HURT;
static Mix_Chunk *SOUND_ENEMY_DEATH;
static Mix_Chunk *SOUND_PLAYER_DEATH;

static const f32 GROUNDED_TIME = 0.1f;
static const f32 SPEED_PLAYER = 250;
static const f32 JUMP_VELOCITY = 1350;
static const f32 SPEED_ENEMY_LARGE = 80;
static const f32 SPEED_ENEMY_SMALL = 100;
static const f32 HEALTH_ENEMY_LARGE = 7;
static const f32 HEALTH_ENEMY_SMALL = 3;

typedef enum collision_layer {
	COLLISION_LAYER_PLAYER = 1,
	COLLISION_LAYER_ENEMY = 1 << 1,
	COLLISION_LAYER_TERRAIN = 1 << 2,
	COLLISION_LAYER_ENEMY_PASSTHROUGH = 1 << 3,
    COLLISION_LAYER_PROJECTILE = 1 << 4,
} Collision_Layer;

typedef enum weapon_type {
    WEAPON_TYPE_SHOTGUN,
    WEAPON_TYPE_PISTOL,
    WEAPON_TYPE_REVOLVER,
    WEAPON_TYPE_SMG,
    WEAPON_TYPE_ROCKET_LAUNCHER,
    WEAPON_TYPE_COUNT,
} Weapon_Type;

typedef enum projectile_type {
    PROJECTILE_TYPE_SMALL,
    PROJECTILE_TYPE_LARGE,
    PROJECTILE_TYPE_ROCKET,
} Projectile_Type;

typedef struct weapon {
    f32 fire_rate; // Time between shots.
    f32 recoil;
    f32 projectile_speed;
    Projectile_Type projectile_type;
    vec2 sprite_size;
    vec2 sprite_offset;
    usize projectile_animation_id;
    Mix_Chunk *sfx;
} Weapon;

static Weapon weapons[WEAPON_TYPE_COUNT] = {0};

static f32 render_width;
static f32 render_height;
static u32 texture_slots[8] = {0};

static Weapon_Type weapon_type = WEAPON_TYPE_PISTOL;
static bool should_quit = false;
static bool player_is_grounded = false;
static usize anim_player_walk_id;
static usize anim_player_idle_id;
static usize anim_enemy_small_id;
static usize anim_enemy_large_id;
static usize anim_enemy_small_enraged_id;
static usize anim_enemy_large_enraged_id;
static usize anim_fire_id;
static usize anim_projectile_small_id;

static usize player_id;

static f32 ground_timer = 0;
static f32 shoot_timer = 0;
static f32 spawn_timer = 0;

static u8 enemy_mask = COLLISION_LAYER_PLAYER | COLLISION_LAYER_TERRAIN;
static u8 player_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_TERRAIN | COLLISION_LAYER_ENEMY_PASSTHROUGH;
static u8 fire_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_PLAYER;
static u8 projectile_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_TERRAIN;

void projectile_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
        Entity *projectile = entity_get(self->entity_id);
        Entity *enemy = entity_get(other->entity_id);
        if (projectile->animation_id == anim_projectile_small_id) {
            if (entity_damage(other->entity_id, 1)) {
                audio_sound_play(SOUND_ENEMY_DEATH);
            }
        }
        audio_sound_play(SOUND_HURT);
	}
}

void projectile_on_hit_static(Body *self, Static_Body *other, Hit hit) {
        Entity *projectile = entity_get(self->entity_id);
        if (projectile->animation_id == anim_projectile_small_id) {
            audio_sound_play(SOUND_SHOOT);
        }
        entity_destroy(self->entity_id);
}

static void spawn_projectile(Projectile_Type projectile_type) {
    Weapon weapon = weapons[weapon_type];
    Entity *player = entity_get(player_id);
    Body *body = physics_body_get(player->body_id);
    Animation *animation = animation_get(player->animation_id);
    bool is_flipped = animation->is_flipped;
    vec2 velocity = {is_flipped ? -weapon.projectile_speed : weapon.projectile_speed, 0};

    entity_create(body->aabb.position, weapon.sprite_size, weapon.sprite_offset, velocity, COLLISION_LAYER_PROJECTILE, projectile_mask, true, weapon.projectile_animation_id, projectile_on_hit, projectile_on_hit_static);
    audio_sound_play(weapon.sfx);
}

static void input_handle(Body *body_player) {
	if (global.input.escape) {
		should_quit = true;
	}

    Animation *walk_anim = animation_get(anim_player_walk_id);
    Animation *idle_anim = animation_get(anim_player_idle_id);

	f32 velx = 0;
	f32 vely = body_player->velocity[1];

	if (global.input.right) {
		velx += SPEED_PLAYER;
        walk_anim->is_flipped = false;
        idle_anim->is_flipped = false;
	}

	if (global.input.left) {
		velx -= SPEED_PLAYER;
        walk_anim->is_flipped = true;
        idle_anim->is_flipped = true;
	}

	if (global.input.up && player_is_grounded) {
		player_is_grounded = false;
		vely = JUMP_VELOCITY;
		audio_sound_play(SOUND_JUMP);
	}

	body_player->velocity[0] = velx;
	body_player->velocity[1] = vely;

    if (global.input.shoot && shoot_timer <= 0) {
        Weapon weapon = weapons[weapon_type];
        shoot_timer = weapon.fire_rate;
        spawn_projectile(weapon.projectile_type);
    }
}

void player_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
	}
}

void player_on_hit_static(Body *self, Static_Body *other, Hit hit) {
	if (hit.normal[1] > 0) {
		player_is_grounded = true;
	}
}

void enemy_small_on_hit_static(Body *self, Static_Body *other, Hit hit) {
    Entity *entity = entity_get(self->entity_id);

	if (hit.normal[0] > 0) {
        if (entity->is_enraged) {
            self->velocity[0] = SPEED_ENEMY_SMALL * 1.5f;
        } else {
            self->velocity[0] = SPEED_ENEMY_SMALL;
        }
	}

	if (hit.normal[0] < 0) {
        if (entity->is_enraged) {
            self->velocity[0] = -SPEED_ENEMY_SMALL * 1.5f;
        } else {
            self->velocity[0] = -SPEED_ENEMY_SMALL;
        }
	}
}

void enemy_large_on_hit_static(Body *self, Static_Body *other, Hit hit) {
    Entity *entity = entity_get(self->entity_id);

	if (hit.normal[0] > 0) {
        if (entity->is_enraged) {
            self->velocity[0] = SPEED_ENEMY_LARGE * 1.5f;
        } else {
            self->velocity[0] = SPEED_ENEMY_LARGE;
        }
	}

	if (hit.normal[0] < 0) {
        if (entity->is_enraged) {
            self->velocity[0] = -SPEED_ENEMY_LARGE * 1.5f;
        } else {
            self->velocity[0] = -SPEED_ENEMY_LARGE;
        }
	}
}

void spawn_enemy(bool is_small, bool is_enraged, bool is_flipped) {
    f32 spawn_x = is_flipped ? render_width : 0;
    vec2 position = {spawn_x, (render_height - 64)};
    f32 speed = SPEED_ENEMY_LARGE;
    vec2 size = {20, 20};
    vec2 sprite_offset = {0, 10};
    usize animation_id = anim_enemy_large_id;
    On_Hit_Static on_hit_static = enemy_large_on_hit_static;

    if (is_small) {
        size[0] = 12;
        size[1] = 12;
        sprite_offset[0] = 0;
        sprite_offset[1] = 6;
        animation_id = anim_enemy_small_id;
        on_hit_static = enemy_small_on_hit_static;
        speed = SPEED_ENEMY_SMALL;
    }

    if (is_enraged) {
        speed *= 1.5;
        animation_id = is_small ? anim_enemy_small_enraged_id : anim_enemy_large_enraged_id;
    }

    vec2 velocity = {is_flipped ? -speed : speed, 0};
    usize id = entity_create(position, size, sprite_offset, velocity, COLLISION_LAYER_ENEMY, enemy_mask, false, animation_id, NULL, on_hit_static);
    Entity *entity = entity_get(id);
    entity->is_enraged = is_enraged;
}

void fire_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
        if (other->is_active) {
            Entity *enemy = entity_get(other->entity_id);
            bool is_small = enemy->animation_id == anim_enemy_small_id || enemy->animation_id == anim_enemy_small_enraged_id;
            bool is_flipped = rand() % 100 >= 50;
            spawn_enemy(is_small, true, is_flipped);
            entity_destroy(other->entity_id);
        }
	} else if (other->collision_layer == COLLISION_LAYER_PLAYER) {
        reset();
    }
}

void reset(void) {
    audio_music_play(MUSIC_STAGE_1);

    physics_reset();
    entity_reset();

    ground_timer = 0;
    spawn_timer = 0;
    shoot_timer = 0;

	player_id = entity_create((vec2){100, 200}, (vec2){24, 24}, (vec2){0, 0}, (vec2){0, 0}, COLLISION_LAYER_PLAYER, player_mask, false, (usize)-1, player_on_hit, player_on_hit_static);

    // Init level.
	{
		physics_static_body_create((vec2){render_width * 0.5, render_height - 16}, (vec2){render_width, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.25 - 16, 16}, (vec2){render_width * 0.5 - 32, 48}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.75 + 16, 16}, (vec2){render_width * 0.5 - 32, 48}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){16, render_height * 0.5 - 3 * 32}, (vec2){32, render_height}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width - 16, render_height * 0.5 - 3 * 32}, (vec2){32, render_height}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){32 + 64, render_height - 32 * 3 - 16}, (vec2){128, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width - 32 - 64, render_height - 32 * 3 - 16}, (vec2){128, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.5, render_height - 32 * 3 - 16}, (vec2){192, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.5, 32 * 3 + 24}, (vec2){448, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){16, render_height - 64}, (vec2){32, 64}, COLLISION_LAYER_ENEMY_PASSTHROUGH);
		physics_static_body_create((vec2){render_width - 16, render_height - 64}, (vec2){32, 64}, COLLISION_LAYER_ENEMY_PASSTHROUGH);
        
        physics_trigger_create((vec2){render_width * 0.5, -4}, (vec2){64, 8}, 0, fire_mask, fire_on_hit);
	}

    entity_create((vec2){render_width * 0.5, 0}, (vec2){32, 64}, (vec2){0, 0}, (vec2){0, 0}, 0, 0, true, anim_fire_id, NULL, NULL);
    entity_create((vec2){render_width * 0.5 + 16, -16}, (vec2){32, 64}, (vec2){0, 0}, (vec2){0, 0}, 0, 0, true, anim_fire_id, NULL, NULL);
    entity_create((vec2){render_width * 0.5 - 16, -16}, (vec2){32, 64}, (vec2){0, 0}, (vec2){0, 0}, 0, 0, true, anim_fire_id, NULL, NULL);
}

int main(int argc, char *argv[]) {
	time_init(60);
	SDL_Window *window = render_init();
	config_init();
	physics_init();
	entity_init();
	animation_init();
	audio_init();

	audio_sound_load(&SOUND_JUMP, "engine_from_scratch/assets/jump.wav");
	audio_sound_load(&SOUND_SHOOT, "engine_from_scratch/assets/shoot.wav");
	audio_sound_load(&SOUND_BULLET_HIT_WALL, "engine_from_scratch/assets/bullet_hit_wall.wav");
	audio_sound_load(&SOUND_HURT, "engine_from_scratch/assets/hurt.wav");
	audio_sound_load(&SOUND_ENEMY_DEATH, "engine_from_scratch/assets/enemy_death.wav");
	audio_sound_load(&SOUND_PLAYER_DEATH, "engine_from_scratch/assets/player_death.wav");
	audio_music_load(&MUSIC_STAGE_1, "engine_from_scratch/assets/breezys_mega_quest_2_stage_1.mp3");

	i32 window_width, window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);
	render_width = window_width / render_get_scale();
	render_height = window_height / render_get_scale();

	Sprite_Sheet sprite_sheet_player;
	Sprite_Sheet sprite_sheet_map;
	Sprite_Sheet sprite_sheet_enemy_small;
	Sprite_Sheet sprite_sheet_enemy_large;
	Sprite_Sheet sprite_sheet_props;
    Sprite_Sheet sprite_sheet_fire;
	render_sprite_sheet_init(&sprite_sheet_player, "engine_from_scratch/assets/player.png", 24, 24);
    render_sprite_sheet_init(&sprite_sheet_map, "engine_from_scratch/assets/map.png", 640, 360);
    render_sprite_sheet_init(&sprite_sheet_enemy_small, "engine_from_scratch/assets/enemy_small.png", 24, 24);
    render_sprite_sheet_init(&sprite_sheet_enemy_large, "engine_from_scratch/assets/enemy_large.png", 40, 40);
    render_sprite_sheet_init(&sprite_sheet_props, "engine_from_scratch/assets/props_16x16.png", 16, 16);
    render_sprite_sheet_init(&sprite_sheet_fire, "engine_from_scratch/assets/fire.png", 32, 64);

	usize adef_player_walk_id = animation_definition_create(&sprite_sheet_player, 0.1, 0, (u8[]){1, 2, 3, 4, 5, 6, 7}, 7);
	usize adef_player_idle_id = animation_definition_create(&sprite_sheet_player, 0, 0, (u8[]){0}, 1);
	anim_player_walk_id = animation_create(adef_player_walk_id, true);
	anim_player_idle_id = animation_create(adef_player_idle_id, false);

    usize adef_enemy_small_id = animation_definition_create(&sprite_sheet_enemy_small, 0.1, 1, (u8[]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
    usize adef_enemy_large_id = animation_definition_create(&sprite_sheet_enemy_large, 0.1, 1, (u8[]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
    usize adef_enemy_small_enraged_id = animation_definition_create(&sprite_sheet_enemy_small, 0.1, 0, (u8[]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
    usize adef_enemy_large_enraged_id = animation_definition_create(&sprite_sheet_enemy_large, 0.1, 0, (u8[]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
    anim_enemy_small_id = animation_create(adef_enemy_small_id, true);
    anim_enemy_large_id = animation_create(adef_enemy_large_id, true);
    anim_enemy_small_enraged_id = animation_create(adef_enemy_small_enraged_id, true);
    anim_enemy_large_enraged_id = animation_create(adef_enemy_large_enraged_id, true);

    usize adef_fire_id = animation_definition_create(&sprite_sheet_fire, 0.1, 0, (u8[]){0, 1, 2, 3, 4, 5, 6, 7}, 7);
    anim_fire_id = animation_create(adef_fire_id, true);

    usize adef_projectile_small_id = animation_definition_create(&sprite_sheet_props, 1, 0, (u8[]){0}, 1);
    anim_projectile_small_id = animation_create(adef_projectile_small_id, false);

    // Init weapons.
    weapons[WEAPON_TYPE_PISTOL] = (Weapon){
        .projectile_type = PROJECTILE_TYPE_SMALL,
            .projectile_speed = 200,
            .fire_rate = 0.1,
            .recoil = 2.0,
            .projectile_animation_id = anim_projectile_small_id,
            .sprite_size = {16, 16},
            .sprite_offset = {0, 0},
            .sfx = SOUND_SHOOT
    };

    reset();

	while (!should_quit) {
		time_update();

		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				should_quit = true;
				break;
			default:
				break;
			}
		}

        shoot_timer -= global.time.delta;
        spawn_timer -= global.time.delta;
        ground_timer -= global.time.delta;

		Entity *player = entity_get(player_id);
		Body *body_player = physics_body_get(player->body_id);

		if (body_player->velocity[0] != 0) {
            player->animation_id = anim_player_walk_id;
		} else {
            player->animation_id = anim_player_idle_id;
		}

		input_update();
		input_handle(body_player);
		physics_update();

		animation_update(global.time.delta);

		// Spawn enemies.
		{
			if (spawn_timer <= 0) {
				spawn_timer = (f32)((rand() % 200) + 200) / 100.f;

				spawn_timer *= 0.2;

				bool is_flipped = rand() % 100 >= 50;
				bool is_small = rand() % 100 > 18;

				f32 spawn_x = is_flipped ? 540 : 100;
                spawn_enemy(is_small, false, is_flipped);
			}
		}

		render_begin();

        // Render terrain/map.
        render_sprite_sheet_frame(&sprite_sheet_map, 0, 0, (vec2){render_width / 2.0, render_height / 2.0}, false, (vec4){1, 1, 1, 0.2}, texture_slots);

        // Debug render bounding boxes.
        {
            for (usize i = 0; i < entity_count(); ++i) {
                Entity *entity = entity_get(i);
                Body *body = physics_body_get(entity->body_id);

                if (body->is_active) {
                    render_aabb((f32*)body, TURQUOISE);
                } else {
                    render_aabb((f32*)body, RED);
                }
            }

            for (usize i = 0; i < physics_static_body_count(); ++i) {
                render_aabb((f32*)physics_static_body_get(i), WHITE);
            }
        }

		// Render animated entities...
		for (usize i = 0; i < entity_count(); ++i) {
			Entity *entity = entity_get(i);
			if (!entity->is_active || entity->animation_id == (usize)-1) {
				continue;
			}

			Body *body = physics_body_get(entity->body_id);
			Animation *anim = animation_get(entity->animation_id);

			if (body->velocity[0] < 0) {
				anim->is_flipped = true;
			} else if (body->velocity[0] > 0) {
				anim->is_flipped = false;
			}

            vec2 pos;

            vec2_add(pos, body->aabb.position, entity->sprite_offset);
            animation_render(anim, pos, WHITE, texture_slots);
		}

		render_end(window, texture_slots);

		time_update_late();
	}

	return 0;
}