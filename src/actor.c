#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <math.h>
#include <stdio.h>
#include "animation.h"
#include "actor.h"
#include "jb_types.h"
#include "constants.h"
#include "tiles.h"
#include "sounds.h"



Direction opposite_directions[ 4 ] = { DIR_UP, DIR_DOWN, DIR_RIGHT, DIR_LEFT };

void actor_set_current_tile( Actor *actor ) {
    actor->current_tile.x = ( ( actor->world_position.x + TILE_SIZE / 2 ) / TILE_SIZE ) ;
    actor->current_tile.y = ( ( ( actor->world_position.y + TILE_SIZE / 2 ) - (TILE_SIZE * 2 ) ) / TILE_SIZE ) ;
}

Actor *init_actor( SDL_Point initial_tile, SDL_Point tilemap_offset, float base_speed, float speed_multp ) {
    Actor *actor;
    actor = ( Actor *) malloc( sizeof( Actor ) );

    //SDL_Point screen_point = tile_grid_point_to_screen_point( initial_tile, tilemap_offset);

    actor->world_position.x = initial_tile.x * TILE_SIZE;
    actor->world_position.y = initial_tile.y * TILE_SIZE;

    actor->world_center_point.x = ( int ) actor->world_position.x + ( ACTOR_SIZE / 2 );
    actor->world_center_point.y = ( int ) actor->world_position.y + ( ACTOR_SIZE / 2 );

    actor->world_top_sensor.x = actor->world_position.x + ( ACTOR_SIZE / 2 );
    actor->world_top_sensor.y = actor->world_position.y;

    actor->world_bottom_sensor.x = actor->world_position.x + ( ACTOR_SIZE / 2 );
    actor->world_bottom_sensor.y = actor->world_position.y + ACTOR_SIZE;

    actor->world_left_sensor.x = actor->world_position.x;
    actor->world_left_sensor.y = actor->world_position.y + ( ACTOR_SIZE / 2 );

    actor->world_right_sensor.x = actor->world_position.x + ACTOR_SIZE;
    actor->world_right_sensor.y = actor->world_position.y + ( ACTOR_SIZE / 2 );    

    actor->direction = DIR_NONE;

    //actor_set_current_tile( actor );
    actor->current_tile = initial_tile;
    actor->next_tile = actor->current_tile;
    actor->next_tile = actor->current_tile;

    actor->base_speed = base_speed;
    actor->speed_multp = speed_multp;

    return actor;
}

void actor_align_world_data_based_on_world_position( Actor *actor ) {
    actor->world_center_point.x = ( int ) actor->world_position.x + ( TILE_SIZE / 2 );
    actor->world_center_point.y = ( int ) actor->world_position.y + ( TILE_SIZE / 2 );

    actor->current_tile.x = ( ( actor->world_position.x + TILE_SIZE / 2 ) / TILE_SIZE ) ;
    actor->current_tile.y = ( ( ( actor->world_position.y + TILE_SIZE / 2 ) ) / TILE_SIZE ) ;

    actor->world_top_sensor.x = actor->world_position.x + ( TILE_SIZE / 2 );
    actor->world_top_sensor.y = actor->world_position.y;

    actor->world_bottom_sensor.x = actor->world_position.x + ( TILE_SIZE / 2 );
    actor->world_bottom_sensor.y = actor->world_position.y + TILE_SIZE;

    actor->world_left_sensor.x = actor->world_position.x;
    actor->world_left_sensor.y = actor->world_position.y + ( TILE_SIZE / 2 );

    actor->world_right_sensor.x = actor->world_position.x + TILE_SIZE;
    actor->world_right_sensor.y = actor->world_position.y + ( TILE_SIZE / 2 );
}

void actor_reset_data( Actor *actor, SDL_Point initial_tile ) {
    actor->world_position.x = initial_tile.x * TILE_SIZE;
    actor->world_position.y = initial_tile.y * TILE_SIZE;

    actor->world_center_point.x = ( int ) actor->world_position.x + ( ACTOR_SIZE / 2 );
    actor->world_center_point.y = ( int ) actor->world_position.y + ( ACTOR_SIZE / 2 );

    actor->world_top_sensor.x = actor->world_position.x + ( ACTOR_SIZE / 2 );
    actor->world_top_sensor.y = actor->world_position.y;

    actor->world_bottom_sensor.x = actor->world_position.x + ( ACTOR_SIZE / 2 );
    actor->world_bottom_sensor.y = actor->world_position.y + ACTOR_SIZE;

    actor->world_left_sensor.x = actor->world_position.x;
    actor->world_left_sensor.y = actor->world_position.y + ( ACTOR_SIZE / 2 );

    actor->world_right_sensor.x = actor->world_position.x + ACTOR_SIZE;
    actor->world_right_sensor.y = actor->world_position.y + ( ACTOR_SIZE / 2 );    

    actor->direction = DIR_NONE;

    actor->current_tile = initial_tile;
    actor->next_tile = actor->current_tile;
    actor->next_tile = actor->current_tile;
}


void pac_collect_dot( Actor *pacmonster, char dots[ TILE_ROWS ][ TILE_COLS ], unsigned int *num_dots, Score *score, SDL_Renderer *renderer ) {
    if( dots[ pacmonster->current_tile.y ][ pacmonster->current_tile.x ] == 'x') {
        if( !Mix_Playing( PAC_CHOMP_CHANNEL ) ) {
            Mix_PlayChannel( PAC_CHOMP_CHANNEL, g_PacChompSound, 0 );
        }
        // get rid of dot marker
        dots[ pacmonster->current_tile.y ][ pacmonster->current_tile.x ] = ' ';

        unsigned int n = *num_dots - 1;
        *num_dots = n;
        
        score->score_number += 20;
        
        snprintf( score->score_text, 32, "Score : %d", score->score_number );
        SDL_Surface *score_surface = TTF_RenderText_Solid( score->font, score->score_text, score->score_color );

        SDL_DestroyTexture( score->score_texture );
        score->score_texture = SDL_CreateTextureFromSurface( renderer, score_surface );
        score->score_render_dst_rect.x = 10;
        score->score_render_dst_rect.y = 10;
        score->score_render_dst_rect.w = score_surface->w;
        score->score_render_dst_rect.h = score_surface->h;

        SDL_FreeSurface( score_surface );

    }

}


